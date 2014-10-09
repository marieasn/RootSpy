// $Id$
//
//    File: DRootSpy.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>

#include <iostream>
#include<sstream>
#include <cmath>
using namespace std;

#include <TROOT.h>
#include <TDirectoryFile.h>
#include <TClass.h>
#include <TH1.h>
#include <TMessage.h>
#include <pthread.h>
#include <TTree.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TFile.h>
#include <TMemFile.h>

#include "DRootSpy.h"
#ifndef _DBG_
#define _DBG_ cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl;
#endif
void *ReturnFinalsC(void * ptr);

//...................................................
// This is used to temporarily store simple macro
// definitions until a DRootSpy object is created
typedef struct{
	string name;
	string path;
	string macro_data;
}RS_QUEUED_MACRO_t;
//...................................................


// --------- GLOBALS ----------------
DRootSpy *gROOTSPY = NULL;
sem_t RootSpy_final_sem;
pthread_mutex_t *gROOTSPY_MUTEX = NULL;
vector<RS_QUEUED_MACRO_t> QUEUED_MACROS;

//...................................................
// REGISTER_ROOTSPY_MACRO
//
// The following allows us to optionally register macros iff the symbol
// REGISTER_ROOTSPY_MACRO can be found by dlsym. This way, plugins with
// macros can be used with or without the rootspy plugin attached. The
// user's build system (SBMS for the online packages in GlueX) can be
// made to automatically copy scripts into strings that are compiled
// along with a call to REGISTER_ROOTSPY_MACRO so that the whole system
// is automatic.
extern "C"{
void REGISTER_ROOTSPY_MACRO(string name, string path, string macro_data){
	// If a DRootSpy object already exists, register the macro right away
	// otherwise, just copy it to a global container to be regsitered later
	if(gROOTSPY){
		gROOTSPY->RegisterMacro(name, path, macro_data);
	}else{
		RS_QUEUED_MACRO_t rsqm = {name, path, macro_data};
		QUEUED_MACROS.push_back(rsqm);
	}
}
};
//...................................................


//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
//TODO: documentation comment.
DRootSpy::DRootSpy(string udl)
{
	Initialize(NULL, udl);
}

//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
//TODO: documentation comment.
DRootSpy::DRootSpy(pthread_mutex_t *mutex, string udl)
{
	Initialize(mutex, udl);
}

//---------------------------------
// Initialize
//
/// This is called from the constructors and should not be
/// called from anywhere else.
//---------------------------------
void DRootSpy::Initialize(pthread_mutex_t *mutex, string myUDL)
{

	// Initialize the gROOTSPY_MUTEX global either with the user
	// supplied mutex or by allocating our own.
	if(mutex){
		// User provided mutex for locking ROOT global
		gROOTSPY_MUTEX = mutex;
		own_gROOTSPY_MUTEX = false;
	}else{
		// Create our own mutex. Make it error checking.
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		gROOTSPY_MUTEX = new pthread_mutex_t;
		pthread_mutex_init(gROOTSPY_MUTEX, &attr);
		own_gROOTSPY_MUTEX = true;
	}

	// Initialize member data
	pthread_mutex_lock(gROOTSPY_MUTEX);
	hist_dir = gDirectory;
	pthread_mutex_unlock(gROOTSPY_MUTEX);
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);
	
	// Determine UDL for connecting to cMsg server
	// The preference is given is:
	//  1. udl passed to us (if not set to "<default">)
	//  2. environment variable ROOTSPY_UDL if set
	//  3. hardwired UDL (cMsg://localhost/cMsg/rootspy)
	//
	// If "<default>" is specified, then check for 2 then 3
	if(myUDL == "<default>"){
		const char *ROOTSPY_UDL = getenv("ROOTSPY_UDL");
		if(ROOTSPY_UDL){
			myUDL = ROOTSPY_UDL;
		}else{
			myUDL = "cMsg://127.0.0.1/cMsg/rootspy";
		}
	}

	// Connect to cMsg system
	string myName = myname;
	string myDescr = "Access ROOT objects in JANA program";
	cMsgSys = new cMsg(myUDL,myName,myDescr);      // the cM#include <pthread.h>sg system object, where
	try {                                    //  all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl<<endl<<endl;
		return;
	}

	cout<<"---------------------------------------------------"<<endl;
	cout<<"rootspy name: \""<<myname<<"\""<<endl;
	cout<<"---------------------------------------------------"<<endl;

	// Subscribe to generic rootspy info requests
	subscription_handles.push_back(cMsgSys->subscribe("rootspy", "*", this, NULL));

	// Subscribe to rootspy requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Broadcast that we're here to anyone already listening
	cMsgMessage ImAlive;
	ImAlive.setSubject("rootspy");
	ImAlive.setType(myname);
	cMsgSys->send(&ImAlive);

	// semaphore for handling sending final histograms
	int err = sem_init(&RootSpy_final_sem, 0, 1);
	
	finalhists = NULL;

	//create a thread to simulate what the environment of using the
	//finalHist msg and vector would be like. Note that if an error
	// occurs in the sem_init call above, then we should NOT launch
	// the thread here because the semaphore will be unavailable.
	// This is always the case for Mac OS X which does not support
	// unnamed semaphores, but does define the sem_init function
	// such that it always returns -1.
	if(err == 0){
		pthread_create(&mythread, NULL, ReturnFinalsC, this);
	}else{
		cout << "Unable to create semephore. Final Histograms feature disabled." << endl;
	}
	
	// Register any macros that were queued up before the DRootSpy object was created
	for(unsigned int i=0; i<QUEUED_MACROS.size(); i++){
		RS_QUEUED_MACRO_t &rsqm = QUEUED_MACROS[i];
		RegisterMacro(rsqm.name, rsqm.path, rsqm.macro_data);
	}
	QUEUED_MACROS.clear();
	
	VERBOSE=1;
	const char *ROOTSPY_VERBOSE = getenv("ROOTSPY_VERBOSE");
	if(ROOTSPY_VERBOSE) VERBOSE = atoi(ROOTSPY_VERBOSE);
	
	gROOTSPY = this;
}

//---------------------------------
// ~DRootSpy    (Destructor)
//---------------------------------
DRootSpy::~DRootSpy()
{
	// Unsubscribe
	for(unsigned int i=0; i<subscription_handles.size(); i++){
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();
	cMsgSys->disconnect();

	sem_destroy(&RootSpy_final_sem);
	
	if(own_gROOTSPY_MUTEX && gROOTSPY_MUTEX!=NULL){
		delete gROOTSPY_MUTEX;
		gROOTSPY_MUTEX = NULL;
	}
}

//---------------------------------
// callback
//---------------------------------
//TODO: documentation comment.
void DRootSpy::callback(cMsgMessage *msg, void *userObject) {

	if(!msg)return;
	if(VERBOSE>1) cout<<"Received message --  Subject:"<<msg->getSubject()
			<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;

	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();

	// The actual command is always sent in the text of the message
	string cmd = msg->getText();

	// Prepare to send a response (this may or may not
	//be needed below, depending on the command)
	cMsgMessage *response;
	if(msg->isGetRequest()) {
	    if(VERBOSE>1) _DBG_ << "responding to direct message..." << endl;
	    response = msg->response();
	} else {
	    response = new cMsgMessage();
	}
	response->setSubject(sender);
	response->setType(myname);

	// Dispatch command 
	if(cmd == "who's there?") {
		response->setText("I am here");
		cMsgSys->send(response);
		//delete msg;
		//return;
	} else 	if(cmd == "list hists") {
		listHists(*response);
		//delete msg;
		//return;
	} else 	if(cmd == "get hist") {
		// Get name of requested histogram
		string hnamepath = msg->getString("hnamepath");
		getHist(*response, hnamepath);
		//delete msg;
		//return;
	} else 	if(cmd == "get tree") {
		// Get name of requested tree
		string name = msg->getString("tree_name");
		string path = msg->getString("tree_path");
		int64_t nentries = msg->getInt64("num_entries");
		getTree(*response, name, path, nentries);
		//delete msg;
		//return;
	} else 	if(cmd == "tree info") {
	    // For a normal request, we send all the information about the TTrees in memory,
	    // including the information about the branches, with one TTree per message.
	    // For a synchronous request, we can only send one response, so we send info about
	    // all of the trees without branch information.  The structure of these messages
	    // is slightly different, so the receiver should handle that.
	    if(msg->isGetRequest()) {
		treeInfoSync(*response, sender);
	    } else {
		treeInfo(sender);
		delete msg;
		return;
	    }
	} else 	if(cmd == "list macros") {
		listMacros(*response);
	} else 	if(cmd == "get macro") {
		// Get name of requested histogram
		string hnamepath = msg->getString("hnamepath");
		if(VERBOSE>1) _DBG_ << "sending out macro " << hnamepath << endl;
		getMacro(*response, hnamepath);
	} else 	if(cmd == "provide final") {
		finalhists = msg->getStringVector("hnamepaths");
		finalsender = msg->getType();
		sem_post(&RootSpy_final_sem);
		if(VERBOSE>1) cerr<<"got finalhists"<<endl;
		delete msg;
		return;
	}

	// if we've handled a synchronous request, be sure to flush the output buffer
	if(msg->isGetRequest()) {
	    if(VERBOSE>1) _DBG_ << "flushing queues!" << endl;
	    cMsgSys->flush();
	}else{
		 delete response;
	}

	delete msg;

}

//---------------------------------
// listHists
//---------------------------------
// Save the top-level directory used to form the list of histograms.
// This is needed because we want to "cd" into the path relative to
// this directory when servicing a "get hist" request below. Specfically,
// using gROOT->cd(PATH) will not work if the histograms are being
// written to a file. At the same time, we don't want to assume that
// the program we're attached to doesn't ever change the value of
// gDirectory while running so we save it here. Note that this doesn't
// actually guarantee it will work due to multiple threads, but it
// at least will for the simplest cases.
void DRootSpy::listHists(cMsgMessage &response) 
{

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	hist_dir = gDirectory;
	
	// Add histograms to list, recursively traversing ROOT directories
	vector<hinfo_t> hinfos;
	addRootObjectsToList(hist_dir, hinfos);
	
	// Release lock on ROOT global
	pthread_mutex_unlock(gROOTSPY_MUTEX);
	
	// If any histograms were found, copy their info into the message
	if(hinfos.size()>0) {
		// Copy elements of hinfo objects into individual vectors
		vector<string> hist_names;
		vector<string> hist_types;
		vector<string> hist_paths;
		vector<string> hist_titles;
		for(unsigned int i=0; i<hinfos.size(); i++){
			hist_names.push_back(hinfos[i].name);
			hist_types.push_back(hinfos[i].type);
			hist_paths.push_back(hinfos[i].path);
			hist_titles.push_back(hinfos[i].title);
		}
		response.add("hist_names", hist_names);
		response.add("hist_types", hist_types);
		response.add("hist_paths", hist_paths);
		response.add("hist_titles", hist_titles);
		
	} else if(hinfos.size()==0){
		
		// cMsg insists there be at least one entry in 
		// a vector<string> payload, otherwise it will
		// throw an exception. If we get here, then don't
		// add the empty payloads. Note that by sending
		// empty payloads, this will result in a 
		// message on the client complaining about a
		// mal-formed response.
	}
	
	response.setText("hists list");
	cMsgSys->send(&response);
}

//---------------------------------
// getHist
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getHist(cMsgMessage &response, string &hnamepath) {
	// split hnamepath into histo name and path
	size_t pos = hnamepath.find_last_of("/");
	if(pos == string::npos)return;
	string hname = hnamepath.substr(pos+1, hnamepath.length()-1);
	string path = hnamepath.substr(0, pos);
	if(path[path.length()-1]==':')path += "/";

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	// Get pointer to ROOT object
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(hname.c_str());
	savedir->cd();
	
	// If object not found, release lock on ROOT global and return
	if(!obj){
		pthread_mutex_unlock(gROOTSPY_MUTEX);
		return;
	}

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_OBJECT);
	tm->WriteObject(obj);
	response.setByteArray(tm->Buffer(),tm->Length());
	response.add("hnamepath", hnamepath);
	response.setText("histogram");

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_mutex_unlock(gROOTSPY_MUTEX);

	// Send message containing histogram (asynchronously)
	cMsgSys->send(&response);	
}


//---------------------------------
// listMacros
//---------------------------------
void DRootSpy::listMacros(cMsgMessage &response) 
{
	// If any histograms were found, copy their info into the message
	if(macros.size()>0) {
		vector<string> macro_names;
		vector<string> macro_paths;
		for(map<string,macro_info_t>::iterator macro_itr = macros.begin();
		    macro_itr != macros.end(); macro_itr++) {
			macro_names.push_back(macro_itr->second.name);
			macro_paths.push_back(macro_itr->second.path);
		}
		response.add("macro_names", macro_names);
		response.add("macro_paths", macro_paths);
	}
	
	response.setText("macros list");
	cMsgSys->send(&response);
}

//---------------------------------
// getMacro
//---------------------------------
void DRootSpy::getMacro(cMsgMessage &response, string &hnamepath) {
	// find the macro
	map<string,macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if(the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		return;
	}
	macro_info_t &the_macro = the_macro_itr->second;

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	TDirectory *savedir = gDirectory;
	
	// for the details on TMemFile usage, see getTree()
	TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");
	
	// fill the TMemFile with our payload:
	//  1) TObjString of the macro "code"
	//  2) TObjArray of any histograms used by the macro
	TObjString *macro_str = new TObjString(the_macro.macro.c_str());
	//cerr << " ======= MACRO =======" << endl;
	//cerr << the_macro.macro << endl;
	//cerr << " ======= TObjString MACRO =======" << endl;
	//cerr << macro_str->GetString().Data() << endl;
	macro_str->Write("macro");

	//TObjArray *hists = new TObjArray(the_macro.histograms.size());
	if( the_macro.histograms.size() > 0 ) {
		//TObjArray *hists = new TObjArray();
		//for(unsigned int i=0; i<the_macro.histograms.size(); i++)
		//hists->AddLast(the_macro.histograms[i]);
		//hists->Write("hists");
		for(unsigned int i=0; i<the_macro.histograms.size(); i++)
			the_macro.histograms[i]->Write();
	}
	
	savedir->cd();

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_ANY);
	f->Write();
	//tm->WriteTString(f->GetName());
	tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
	f->CopyTo(*tm);

	if(VERBOSE>1)_DBG_ << " TMemFile length = " << f->GetEND() << endl;

	response.setByteArray(tm->Buffer(),tm->Length());
	response.add("macro_name", the_macro.name);
	response.add("macro_path", the_macro.path);
	response.add("macro_version", the_macro.version);
	response.setText("macro");

	// clean up everything
	if(f) {
		f->Close();
		delete f;
		f = NULL;
	}

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_mutex_unlock(gROOTSPY_MUTEX);

	// Send message containing histogram (asynchronously)
	cMsgSys->send(&response);	
}




//---------------------------------
// getTree
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getTree(cMsgMessage &response, string &name, string &path, int64_t nentries_to_save) {

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	// Get pointer to tree
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(name.c_str());
	savedir->cd();

	// If object not found, release lock on ROOT global and return
	if(!obj){
		pthread_mutex_unlock(gROOTSPY_MUTEX);
		return;
	}

	// Make sure this is a TTree
	if(strcmp(obj->ClassName(), "TTree")){
		cout << "Request for tree \""<<name<<"\" but that's not a TTree (it's a "<<obj->ClassName()<<")" << endl;
		pthread_mutex_unlock(gROOTSPY_MUTEX);
		return;
	}

	// Cast object as TTree and decide how many entries to copy into the tree we're going
	// to package up and send.
	TTree *tree = static_cast<TTree *>(obj);
	int64_t nentries_in_tree = tree->GetEntries();
	int64_t firstentry = nentries_in_tree - nentries_to_save;
	if(firstentry<0 || nentries_to_save<=0) firstentry=0;
	int64_t nentries = nentries_in_tree - firstentry;
	if(VERBOSE>1) cout << "Sending TTree " << tree->GetName() << "  (entries: " << nentries_in_tree << "  sending: " << nentries << ")" << endl;

	// Copy TTree
	TTree *tree_copy = tree;

	// Sending the TTree object seemed to have problems if
	// the tree contained an array (e.g. "data[100]/F" in
	// the branch definition). We write the tree to a TMemFile
	// and then serialize and send that object. This follows the
	// treeClient.C and fastMergeServer.C ROOT tutorials.
	savedir = gDirectory;
	TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");
	
	tree_copy = tree->CloneTree(0);

	savedir->cd();

	if(!tree_copy){
		cout << "Unable to make temporary copy of tree for transport!" << endl;
		if(f) { f->Close(); delete f; f = NULL;}
		pthread_mutex_unlock(gROOTSPY_MUTEX);
		return;
	}

	for(int64_t i=0; i<nentries; i++) {
		tree->GetEntry(firstentry + i);
		tree_copy->Fill();
	}


#if 0
	// if we're given a positive number of events to save
	// then save the last N events in the tree
	if( nentries_to_save > 0 ) {

	    TTree *newtree = tree->CloneTree(0);
	    
	    int64_t istart = nentries-nentries_to_save;
	    if(istart<0) istart=0;
	    cout << "  (only sending " << nentries_to_save - istart << " entries by request)" << endl;
	    for(int64_t i=istart; i<nentries; i++) {
		tree->GetEntry(i);
		newtree->Fill();
	    }
	    obj = (TObject*)newtree;
	}
#endif

	// Serialize object and put it into a response message
	//TMessage *tm = new TMessage(kMESS_OBJECT);
	//tm->WriteObject(tree_copy);

	TMessage *tm = new TMessage(kMESS_ANY);
	f->Write();
	tm->WriteTString(f->GetName());
	tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
	f->CopyTo(*tm);

	response.setByteArray(tm->Buffer(),tm->Length());
	response.add("tree_name", name);
	response.add("tree_path", path);
	response.setText("tree");

	// clean up temporary copy of tree
	if( tree_copy != tree ) {
		// see source for TTree::MergeTrees
		tree->GetListOfClones()->Remove(tree_copy);
		tree_copy->ResetBranchAddresses();
		delete tree_copy;
	}
	if(f) {
		f->Close();
		delete f;
		f = NULL;
	}

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	pthread_mutex_unlock(gROOTSPY_MUTEX);

	cMsgSys->send(&response);

}

//---------------------------------
// treeInfo
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfo(string sender) {

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	hist_dir = gDirectory;
	findTreeObjForMsg(hist_dir, sender);

	// Release ROOT global
	pthread_mutex_unlock(gROOTSPY_MUTEX);
}

//---------------------------------
// treeInfoSync
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfoSync(cMsgMessage &response, string sender) {

	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

    hist_dir = gDirectory;
    vector<string> tree_names, tree_titles, tree_paths;
    findTreeNamesForMsg(hist_dir, tree_names, tree_titles, tree_paths);

	// Look for any open files and search those as well
	TSeqCollection *coll = gROOT->GetListOfFiles();
	for(int i=0; i<coll->GetSize(); i++){
		TObject *obj = coll->At(i);
		TDirectory *dir = dynamic_cast<TDirectory*>(obj);
		if(!dir) continue;
		if(dir==hist_dir) continue; // already handled
		findTreeNamesForMsg(dir, tree_names, tree_titles, tree_paths);
	}
	
	// Release ROOT global
	pthread_mutex_unlock(gROOTSPY_MUTEX);
    
    // build message
	if(!tree_names.empty()){
    response.add("tree_names", tree_names);
    response.add("tree_titles", tree_titles);
    response.add("tree_paths", tree_paths);
	}
    response.setText("tree info");
    cMsgSys->send(&response);
}

//---------------------------------
// addRootObjectsToList
//---------------------------------
//This method recursively traverses through histogram directories making hinfo_t 
//objects out of individual histograms and then adds the info_t objects to a list.
// The gROOTSPY_MUTEX should already be locked before calling this so we should not
// lock it here.
void DRootSpy::addRootObjectsToList(TDirectory *dir, vector<hinfo_t> &hinfos) {
	string path = dir->GetPath();

	TList *list = dir->GetList();
	TIter next(list);
	while(TObject *obj = next()) {
		// Make hinfo_t objects out of the individual histogram information.
		// For now, we only report objects inheriting from TH1.
		TH1 *hHist = dynamic_cast<TH1*>(obj);

		if(hHist) {
			hinfo_t hi;
			hi.name = obj->GetName();
			hi.type = obj->ClassName();
			hi.path = path;
			hi.title = hHist->GetTitle();
			if(hi.title.length() == 0) hi.title = "-"; // cMsg gives "clientThread: error reading message" if title is empty string (??)
			hinfos.push_back(hi);
		}

		// If this happens to be a directory, recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if(subdir!=NULL && subdir!=dir) addRootObjectsToList(subdir, hinfos);
	}
}


//---------------------------------
// findTreeNamesForMsg
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_MUTEX should already be locked before calling this so we should not
// lock it here.
void DRootSpy::findTreeNamesForMsg(TDirectory *dir, vector<string> &tree_names, 
				   vector<string> &tree_titles, vector<string> &tree_paths) 
{
	if(VERBOSE>1)_DBG_ << "Looking for trees in: " << dir->GetName() << endl;

    TList *list = dir->GetList();
    TIter next(list);
    //search through root directories for a tree object.
    while(TObject *obj = next()) {
	TTree *hTree = dynamic_cast<TTree*>(obj);
	if(hTree) {
	    tree_names.push_back(obj->GetName());
	    tree_titles.push_back(obj->GetTitle()); 
	    tree_paths.push_back(dir->GetPath());
	}
	
	// If this happens to be a directory,
	//recall ourself to find objects starting from it.
	TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
	if(subdir != NULL && subdir != dir) {
	    findTreeNamesForMsg(subdir, tree_names, tree_titles, tree_paths);
	}
    }
}



//---------------------------------
// findTreeObjForMsg
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_MUTEX should already be locked before calling this so we should not
// lock it here.
void DRootSpy::findTreeObjForMsg(TDirectory *dir, string sender) {
	TList *list = dir->GetList();
	TIter next(list);
	//search through root directories for a tree object.
	while(TObject *obj = next()) {
		TTree *hTree = dynamic_cast<TTree*>(obj);
		if(hTree) {
			TObjArray *branch_list = hTree->GetListOfBranches();
			vector<string> branch_info;
			//Search through the found tree object for information.
			traverseTree(branch_list, branch_info);
			if(!branch_info.empty()) {
				cMsgMessage response;
				response.setSubject(sender);
				response.setType(myname);
				response.add("tree_name", obj->GetName());
				response.add("tree_title", obj->GetTitle());
				response.add("tree_path", dir->GetPath());
				response.add("branch_info", branch_info);
				response.setText("tree info");
				cMsgSys->send(&response);
			}
		}

		// If this happens to be a directory,
		//recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if(subdir != NULL && subdir != dir) {
			findTreeObjForMsg(subdir, sender);
		}
	}
}

//---------------------------------
// traverseTree
//---------------------------------
//TODO: documentation comment.
// The gROOTSPY_MUTEX should already be locked before calling this so we should not
// lock it here.
void DRootSpy::traverseTree(TObjArray *branch_list, vector<string>  &treeinfo){

	if(!branch_list) return;
	for(int i = 0; i < branch_list->GetSize(); i++) {
		TBranch *branch = dynamic_cast<TBranch*>(branch_list->At(i));
		if(branch) {
//			cout<<"BranchName: "<<branch->GetName()<<endl;
			treeinfo.push_back(string(branch->GetName()));
		} else break;
		traverseTree(branch->GetListOfBranches(), treeinfo);
	}
}

//---------------------------------
// ReturnFinals
//---------------------------------
//Justin Barry 07.08.10
//sdobbs  4/18/13
//TODO: documentation comment.
void DRootSpy::ReturnFinals(void) {

    _DBG_<<"Starting ReturnFinals()..."<<endl;

    // loop waiting for final histogram requests 
    while(true) {
	_DBG_<<"starting final histogram loop"<<endl;

	// make sure that final histograms are filled
	sem_wait(&RootSpy_final_sem);

	_DBG_<<"Got semaphore signal"<<endl;

	if(!finalhists) // sanity check
	    continue;

	_DBG_<<"building final histograms"<<endl;
	_DBG_<<"saving " << finalhists->size() << " histograms..." << endl;

	gDirectory->ls(); // debug info

	// send out all the histograms
	vector<string>::iterator hnamepath = finalhists->begin();
	for(; hnamepath !=finalhists->end(); hnamepath++) {	

	    _DBG_ << "looking for " << *hnamepath << endl;
	    
	    // split hnamepath into histo name and path
	    size_t pos = hnamepath->find_last_of("/");
	    if(pos == string::npos)continue;
	    string hname = hnamepath->substr(pos+1, hnamepath->length()-1);
	    string path = hnamepath->substr(0, pos);
	    if(path[path.length()-1]==':')path += "/";
	    // This seems to work, but is certainly not very thread safe.
	    // perhaps there's a better way...?
	    TDirectory *savedir = gDirectory;
	    // use directory from last time "list hist" request was serviced.
	    hist_dir->cd(path.c_str());
	    TObject *obj = gROOT->FindObject(hname.c_str());
	    savedir->cd();
	    if(!obj)continue;

	    // debugging info
	    _DBG_ << "found item: " << endl;
	    obj->Print();

	    // Serialize object and put it into a response message
	    TMessage *tm = new TMessage(kMESS_OBJECT);
	    tm->WriteObject(obj);
	    
	    // Prepare to send a response
	    cMsgMessage response;
	    response.setSubject(finalsender);
	    response.setType(myname);
	    response.setByteArrayNoCopy(tm->Buffer(),tm->Length());

	    _DBG_ << "sending message of length = " << tm->Length() << endl;

	    response.add("hnamepath", *hnamepath);
	    response.setText("final hists");
	    cMsgSys->send(&response);

	    _DBG_ << "sending out " << *hnamepath << endl;
	}
    }
}


//---------------------------------
// ReturnFinalsC
//---------------------------------
void *ReturnFinalsC(void * ptr) {
	DRootSpy* rsptr = (DRootSpy *) ptr;
	rsptr->ReturnFinals();
	
	return NULL; // avoid compiler warnings
}


//---------------------------------
// RegisterMacro
//---------------------------------
bool DRootSpy::RegisterMacro(string name, string path, string macro_data) 
{
	// validate name
	// right now we don't do anything special...
	if(name == "") {
		_DBG_ << "trying to register a macro with no name!" << endl;
		return false;
	}
	
	cout << "ROOTSPY: Registering macro \""<<name<<"\""<<endl;

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if(path == "") {
		path = "/";
	}

	// add the macro to our list!
	string hnamepath = path + "/" + name;
	macros[hnamepath] = macro_info_t(name,path);
	macros[hnamepath].macro = macro_data;
	
	return true;
}

//---------------------------------
// AddMacroHistogram 
//---------------------------------
bool DRootSpy::AddMacroHistogram(string name, string path, TH1 *the_hist) 
{
	// validate name
	// right now we don't do anything special...
	if(name == "") {
		_DBG_ << "trying to add a histogram to a macro with no name!" << endl;
		return false;
	}

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if(path == "") {
		path = "/";
	}

	// find the macro
	string hnamepath = path + "/" + name;
	map<string,macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if(the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		return false;
	}

	// add the histogram!
	the_macro_itr->second.histograms.push_back( the_hist );

	return true;
}

//---------------------------------
// AddMacroHistogram 
//---------------------------------
bool DRootSpy::AddMacroHistogram(string name, string path, vector<TH1*> the_hists) 
{
	for(vector<TH1*>::iterator hist_itr = the_hists.begin();
	    hist_itr != the_hists.begin(); hist_itr++) {
		if(!AddMacroHistogram(name, path, *hist_itr)) {
			_DBG_ << "error adding histogram to macro " << name << "/" << path << endl;
			return false;
		}
	}

	return true;
}

//---------------------------------
// SetMacroVersion
//---------------------------------
bool DRootSpy::SetMacroVersion(string name, string path, int version_number) 
{
	// validate name
	// right now we don't do anything special...
	if(name == "") {
		_DBG_ << "trying to add a histogram to a macro with no name!" << endl;
		return false;
	}

	// validate path - if blank, then assume we're in the root directory
	// right now we don't do anything else special...
	if(path == "") {
		path = "/";
	}

	// find the macro
	string hnamepath = path + "/" + name;
	map<string,macro_info_t>::iterator the_macro_itr = macros.find(hnamepath);
	if(the_macro_itr == macros.end()) {
		_DBG_ << "Couldn't find macro: " + hnamepath << endl;
		return false;
	}

	// set the value!
	the_macro_itr->second.version = version_number;

	return true;
}

