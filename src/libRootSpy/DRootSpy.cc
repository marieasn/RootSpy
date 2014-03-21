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

#include "DRootSpy.h"
#ifndef _DBG_
#define _DBG_ cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl;
#endif
void *ReturnFinalsC(void * ptr);

// --------- GLOBALS ----------------
sem_t RootSpy_final_sem;

//---------------------------------
// DRootSpy    (Constructor)
//---------------------------------
//TODO: documentation comment.
DRootSpy::DRootSpy(string myUDL) {

	// Initialize member data
	hist_dir = gDirectory;
	
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
	sem_init(&RootSpy_final_sem, 0, 1);
	
	finalhists = NULL;

	//create a thread to simulate what the environment of using the
	//finalHist msg and vector would be like. 
	pthread_create(&mythread, NULL, ReturnFinalsC, this);
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

	sem_destroy(&RootSpy_final_sem);
}

//---------------------------------
// callback
//---------------------------------
//TODO: documentation comment.
void DRootSpy::callback(cMsgMessage *msg, void *userObject) {

	if(!msg)return;
	cout<<"Received message --  Subject:"<<msg->getSubject()
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
	    _DBG_ << "responding to direct message..." << endl;
	    response = msg->response();
	} else
	    response = new cMsgMessage();
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
	} else 	if(cmd == "provide final") {
		finalhists = msg->getStringVector("hnamepaths");
		finalsender = msg->getType();
		sem_post(&RootSpy_final_sem);
		cerr<<"got finalhists"<<endl;
		delete msg;
		return;
	}

	// if we've handled a synchronous request, be sure to flush the output buffer
	if(msg->isGetRequest()) {
	    _DBG_ << "flushing queues!" << endl;
	    cMsgSys->flush();
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
void DRootSpy::listHists(cMsgMessage &response) {

			hist_dir = gDirectory;

			// Add histograms to list, recursively traversing ROOT directories
			vector<hinfo_t> hinfos;
			addRootObjectsToList(hist_dir, hinfos);

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
				// No histograms found. Send back empty lists
				vector<string> hist_names;
				vector<string> hist_types;
				vector<string> hist_paths;
				vector<string> hist_titles;

				//seg fault here.
				_DBG__;
				response.add("hist_names", hist_names);
				response.add("hist_types", hist_types);
				response.add("hist_paths", hist_paths);
				response.add("hist_titles", hist_titles);
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

	// This seems to work, but is certainly not very thread safe.
	// perhaps there's a better way...?
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(hname.c_str());
	savedir->cd();
	if(!obj)return;

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_OBJECT);
	tm->WriteObject(obj);
	response.setByteArrayNoCopy(tm->Buffer(),tm->Length());
	response.add("hnamepath", hnamepath);
	response.setText("histogram");
	cMsgSys->send(&response);
}

//---------------------------------
// getTree
//---------------------------------
//TODO: documentation comment.
void DRootSpy::getTree(cMsgMessage &response, string &name, string &path, int64_t nentries_to_save) {

	// This seems to work, but is certainly not very thread safe.
	// perhaps there's a better way...?
	TDirectory *savedir = gDirectory;
	hist_dir->cd(path.c_str()); // use directory from last time "list hist" request was serviced
	TObject *obj = gROOT->FindObject(name.c_str());
	savedir->cd();
	if(!obj)return;

	// if we're given a positive number of events to save
	// then save the last N events in the tree
	if( nentries_to_save > 0 ) {
	    _DBG_ << "only saving " << nentries_to_save << " entries!" << endl;

	    TTree *oldtree = static_cast<TTree *>(obj);
	    TTree *newtree = oldtree->CloneTree(0);
	    
	    int64_t nentries = oldtree->GetEntries();
	    for(int64_t i=nentries-nentries_to_save; i<nentries; i++) {
		oldtree->GetEntry(i);
		newtree->Fill();
	    }
	    obj = (TObject*)newtree;
	}

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_OBJECT);
	tm->WriteObject(obj);
	response.setByteArrayNoCopy(tm->Buffer(),tm->Length());
        response.add("tree_name", name);
        response.add("tree_path", path);
	response.setText("tree");
	cMsgSys->send(&response);

/*
	// clean up any new trees made
	if( nentries_to_save > 0 ) {
	    delete newtree;
	}
*/
}

//---------------------------------
// treeInfo
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfo(string sender) {
	hist_dir = gDirectory;
	findTreeObjForMsg(hist_dir, sender);
}

//---------------------------------
// treeInfoSync
//---------------------------------
//TODO: documentation comment.
void DRootSpy::treeInfoSync(cMsgMessage &response, string sender) {
    hist_dir = gDirectory;
    vector<string> tree_names, tree_titles, tree_paths;
    findTreeNamesForMsg(hist_dir, tree_names, tree_titles, tree_paths);
    
    // build message
    response.add("tree_names", tree_names);
    response.add("tree_titles", tree_titles);
    response.add("tree_paths", tree_paths);
    response.setText("tree info");
    cMsgSys->send(&response);
}

//---------------------------------
// addRootObjectsToList
//---------------------------------
//This method recursively traverses through histogram directories making hinfo_t 
//objects out of individual histograms and then adds the info_t objects to a list.
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
void DRootSpy::findTreeNamesForMsg(TDirectory *dir, vector<string> &tree_names, 
				   vector<string> &tree_titles, vector<string> &tree_paths) 
{
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


void *ReturnFinalsC(void * ptr) {
	DRootSpy* rsptr = (DRootSpy *) ptr;
	rsptr->ReturnFinals();
	
	return NULL; // avoid compiler warnings
}
