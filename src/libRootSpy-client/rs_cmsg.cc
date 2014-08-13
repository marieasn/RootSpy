// $Id$
//
//    File: rs_cmsg.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include "RootSpy.h"
#include "rs_cmsg.h"
#include "rs_info.h"
#include "tree_info_t.h"
#include "cMsg.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
using namespace std;

#include <TDirectoryFile.h>
#include <TMessage.h>
#include <TH1.h>
#include <TTree.h>
#include <TMemFile.h>
#include <TKey.h>


//// See http://www.jlab.org/Hall-D/software/wiki/index.php/Serializing_and_deserializing_root_objects
//class MyTMessage : public TMessage {
//public:
//   MyTMessage(void *buf, Int_t len) : TMessage(buf, len) { }
//};


// templated join for a string vector - could be templated?
// uses stringstreams 
string join( vector<string>::const_iterator begin, vector<string>::const_iterator end, string delim )
{
  stringstream ss;

  vector<string>::const_iterator it = begin;
  while( it != end ) {
    ss << *(it++);
    if(it != end) 
      ss << delim;
  }

  return ss.str();
}


//---------------------------------
// rs_cmsg    (Constructor)
//---------------------------------
rs_cmsg::rs_cmsg(string &udl, string &name)
{
	// Connect to cMsg system
        is_online = true;
	string myDescr = "Access ROOT objects in a running program";
	cMsgSys = new cMsg(udl, name, myDescr);   	// the cMsg system object, where
	try {                         	           	//  all args are of type string
		cMsgSys->connect(); 
	} catch (cMsgException e) {
		cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl<<endl<<endl;

		// we need to connect to cMsg to run
		is_online = false;
		exit(0);          
		//return;
	}
	
	// Create a unique name for ourself
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "%s_%d", hostname, getpid());
	myname = string(str);

	cout<<"---------------------------------------------------"<<endl;
	cout<<"   cMsg name: \""<<name<<"\""<<endl;
	cout<<"rootspy name: \""<<myname<<"\""<<endl;
	cout<<"---------------------------------------------------"<<endl;

	// Subscribe to generic rootspy info requests
	subscription_handles.push_back(cMsgSys->subscribe("rootspy", "*", this, NULL));

	// Subscribe to rootspy requests specific to us
	subscription_handles.push_back(cMsgSys->subscribe(myname, "*", this, NULL));
	
	// Start cMsg system
	cMsgSys->start();
	
	// Broadcast request for available servers
	PingServers();
}

//---------------------------------
// ~rs_cmsg    (Destructor)
//---------------------------------
rs_cmsg::~rs_cmsg()
{
    if(is_online) {
	// Unsubscribe
	for(unsigned int i=0; i<subscription_handles.size(); i++){
		cMsgSys->unsubscribe(subscription_handles[i]);
	}

	// Stop cMsg system
	cMsgSys->stop();
	cMsgSys->disconnect();
    }
}


//////////////////////////////////////
//// helper functions
//////////////////////////////////////

//---------------------------------
// BuildRequestHists
//---------------------------------
void rs_cmsg::BuildRequestHists(cMsgMessage &msg, string servername)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("list hists");
}

//---------------------------------
// BuildRequestHistogram
//---------------------------------
void rs_cmsg::BuildRequestHistogram(cMsgMessage &msg, string servername, string hnamepath)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get hist");
    msg.add("hnamepath", hnamepath);
}

//---------------------------------
// BuildRequestTreeInfo
//---------------------------------
void rs_cmsg::BuildRequestTreeInfo(cMsgMessage &msg, string servername)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("tree info");
}

//---------------------------------
// BuildRequestTree
//---------------------------------
void rs_cmsg::BuildRequestTree(cMsgMessage &msg, string servername, string tree_name, string tree_path, int64_t num_entries)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get tree");
    msg.add("tree_name", tree_name);
    msg.add("tree_path", tree_path);
    msg.add("num_entries", num_entries);
}

//---------------------------------
// BuildRequestMacroList
//---------------------------------
void rs_cmsg::BuildRequestMacroList(cMsgMessage &msg, string servername)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("list macros");
}

//---------------------------------
// BuildRequestMacro
//---------------------------------
void rs_cmsg::BuildRequestMacro(cMsgMessage &msg, string servername, string hnamepath)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get macro");
    msg.add("hnamepath", hnamepath);
}


//////////////////////////////////////
//// remote commands
//////////////////////////////////////

//---------------------------------
// PingServers
//---------------------------------
void rs_cmsg::PingServers(void)
{
	cMsgMessage whosThere;
	whosThere.setSubject("rootspy");
	whosThere.setType(myname);
	whosThere.setText("who's there?");
	
	cMsgSys->send(&whosThere);
}

//---------------------------------
// RequestHists
//---------------------------------
void rs_cmsg::RequestHists(string servername)
{
	cMsgMessage listHists;
	BuildRequestHists(listHists, servername);	
	cMsgSys->send(&listHists);
}

//---------------------------------
// RequestHistogram
//---------------------------------
void rs_cmsg::RequestHistogram(string servername, string hnamepath)
{
	cMsgMessage requestHist;
	BuildRequestHistogram(requestHist, servername, hnamepath);
	cMsgSys->send(&requestHist);
}

//---------------------------------
// RequestTreeInfo
//---------------------------------
void rs_cmsg::RequestTreeInfo(string servername)
{
	cMsgMessage treeinfo;
	BuildRequestTreeInfo(treeinfo, servername);
	cMsgSys->send(&treeinfo);
}

//---------------------------------
// RequestTree
//---------------------------------
void rs_cmsg::RequestTree(string servername, string tree_name, string tree_path, int64_t num_entries = -1)
{
	cMsgMessage requestTree;
	BuildRequestTree(requestTree, servername, tree_name, tree_path, num_entries);
	cMsgSys->send(&requestTree);
}

//---------------------------------
// RequestMacroList
//---------------------------------
void rs_cmsg::RequestMacroList(string servername)
{
	cMsgMessage listHists;
	BuildRequestMacroList(listHists, servername);	
	cMsgSys->send(&listHists);
}

//---------------------------------
// RequestMacro
//---------------------------------
void rs_cmsg::RequestMacro(string servername, string hnamepath)
{
	cMsgMessage requestHist;
	BuildRequestMacro(requestHist, servername, hnamepath);
	cMsgSys->send(&requestHist);
}



//---------------------------------
// RequestHistogramSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestHistsSync(string servername, timespec_t &myTimeout)
{
    cMsgMessage requestHist;
    BuildRequestHists(requestHist, servername);
    cMsgMessage *response = cMsgSys->sendAndGet(requestHist, &myTimeout);  // check for exception?

    string sender = response->getType();
    RegisterHistList(sender, response);
}

//---------------------------------
// RequestHistogramSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestHistogramSync(string servername, string hnamepath, timespec_t &myTimeout)
{
    cMsgMessage requestHist;
    BuildRequestHistogram(requestHist, servername, hnamepath);
    cMsgMessage *response = cMsgSys->sendAndGet(requestHist, &myTimeout);  // check for exception?

    string sender = response->getType();
    RegisterHistogram(sender, response);
}

//---------------------------------
// RequestTreeInfoSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestTreeInfoSync(string servername, timespec_t &myTimeout)
{
    cMsgMessage treeinfo;
    BuildRequestTreeInfo(treeinfo, servername);

    //_DBG_ << " Sending getTreeInfo message..." << endl;
    cMsgMessage *response = cMsgSys->sendAndGet(treeinfo, &myTimeout);  // check for exception?
    //_DBG_ <<"Received message --  Subject:"<<response->getSubject()
	 // <<" Type:"<<response->getType()<<" Text:"<<response->getText()<<endl;

    string sender = response->getType();
    RegisterTreeInfoSync(sender, response);
}

//---------------------------------
// RequestTreeSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestTreeSync(string servername, string tree_name, string tree_path, timespec_t &myTimeout, int64_t num_entries = -1)
{
    //_DBG_ << "In rs_cmsg::RequestTreeSync()..." << endl;

    cMsgMessage requestTree;
    BuildRequestTree(requestTree, servername, tree_name, tree_path, num_entries);

    //_DBG_ << " Sending getTree message..." << endl;
    cMsgMessage *response = cMsgSys->sendAndGet(requestTree, &myTimeout);  // check for exception?
    //_DBG_ <<"Received message --  Subject:"<<response->getSubject()
	 // <<" Type:"<<response->getType()<<" Text:"<<response->getText()<<endl;
    
    string sender = response->getType();
    //_DBG_ << " received response from  " << sender << endl;
    RegisterTree(sender, response);
}

//---------------------------------
// RequestMacroListSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestMacroListSync(string servername, timespec_t &myTimeout)
{
    cMsgMessage requestHist;
    BuildRequestMacroList(requestHist, servername);
    cMsgMessage *response = cMsgSys->sendAndGet(requestHist, &myTimeout);  // check for exception?

    string sender = response->getType();
    RegisterMacroList(sender, response);
}

//---------------------------------
// RequestMacroSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestMacroSync(string servername, string hnamepath, timespec_t &myTimeout)
{
    cMsgMessage requestHist;
    BuildRequestMacro(requestHist, servername, hnamepath);
    cMsgMessage *response = cMsgSys->sendAndGet(requestHist, &myTimeout);  // check for exception?

    string sender = response->getType();
    RegisterMacro(sender, response);
}




//---------------------------------
// FinalHistogram
//---------------------------------
void rs_cmsg::FinalHistogram(string servername, vector<string> hnamepaths)
{
    cMsgMessage finalhist;
    finalhist.setSubject(servername);
    finalhist.setType(myname);
    finalhist.setText("provide final");
    finalhist.add("hnamepaths", hnamepaths);
    
    cMsgSys->send(&finalhist);
    cerr<<"final hist request sent"<<endl;
}


//---------------------------------
// callback
//---------------------------------
void rs_cmsg::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;
	
	// The convention here is that the message "type" always constains the
	// unique name of the sender and therefore should be the "subject" to
	// which any reponse should be sent.
	string sender = msg->getType();
	if(sender == myname){delete msg; return;} // no need to process messages we sent!
	
	// Look for an entry for this server in RS_INFO map.
	// If it is not there then add it.
	RS_INFO->Lock();
	
	if(RS_INFO->servers.find(sender)==RS_INFO->servers.end()){
		RS_INFO->servers[sender] = server_info_t(sender);
		cout<<"Added \""<<sender<<"\" to server list."<<endl;
	}else{
		// Update lastHeardFrom time for this server
		RS_INFO->servers[sender].lastHeardFrom = time(NULL);
	}
	RS_INFO->Unlock();
	// The actual command is always sent in the text of the message
	if (msg->getText() == "null"){delete msg; return;}
	string cmd = msg->getText();
	// Dispatch command
	bool handled_message = false;
	
	//===========================================================
	if(cmd=="who's there?"){
		// We don't actually respond to these, only servers
		handled_message = true;
	}
	//===========================================================
	if(cmd=="I am here"){
		// We don't really need to do anything here since any message
		// from the server automatically updates the list and lastHeardForm
		// time above.
		handled_message = true;
	}
	//===========================================================
	if(cmd=="hists list"){
		RegisterHistList(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd=="histogram"){	
		RegisterHistogram(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd == "tree info") {
	        RegisterTreeInfo(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd == "tree") {
	        RegisterTree(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd == "macros list") {
	        RegisterMacroList(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd == "macro") {
	        RegisterMacro(sender, msg);
		handled_message = true;
	}
	//===========================================================
	if(cmd=="final hists"){  // save histograms
	    _DBG_<<"received final histograms..."<<endl;
	    RegisterFinalHistogram(sender, msg);
	    handled_message = true;
	}
	//===========================================================
	if(!handled_message){
		_DBG_<<"Received unknown message --  Subject:"<<msg->getSubject()<<" Type:"<<msg->getType()<<" Text:"<<msg->getText()<<endl;
	}
	delete msg;
}

//---------------------------------
// RegisterHistList
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterHistList(string server, cMsgMessage *msg)
{
	/// Copy list of histograms from cMsg into RS_INFO structures.
	bool good_response = true;

	// Get pointers to STL containers that hold the histogram information
	vector<string> *hist_names;
	vector<string> *hist_types;
	vector<string> *hist_paths;
	vector<string> *hist_titles;
	try {                                    //  all args are of type string
		hist_names = msg->getStringVector("hist_names");
		hist_types = msg->getStringVector("hist_types");
		hist_paths = msg->getStringVector("hist_paths");
		hist_titles = msg->getStringVector("hist_titles");
	} catch (cMsgException e) {
		_DBG_<<"Poorly formed response for \"hists list\". Ignoring."<<endl;
		return;
	}

	// Make sure we have valid pointers
	if(!hist_names)good_response = false;
	if(!hist_types)good_response = false;
	if(!hist_paths)good_response = false;
	if(!hist_titles)good_response = false;

	// Make sure containers all have the same number of entries
	if(good_response){
		if (hist_names->size() != hist_types->size())good_response = false;
		if (hist_names->size() != hist_paths->size())good_response = false;
		if (hist_names->size() != hist_titles->size())good_response = false;
	}

	// If the response is incomplete for any reason, then alert user and return.
	if(!good_response){
		_DBG_<<"Poorly formed response for \"hists list\". Ignoring."<<endl;
		return;
	}

	_DBG_ << "got histogram list from " << server << endl;

	// Looks like we got a good response. Loop over histograms and add them to
	// list of hdef_t objects kept in RS_INFO. If there is already an entry
	// for a histogram, verify that the definition matches this new one.
	for(unsigned int i=0; i<hist_names->size(); i++){
	
		// Get name
		string name = (*hist_names)[i];

		// Get path without the preceeding root:
		string path = (*hist_paths)[i];

		size_t pos = path.find_first_of("/");
		if(pos!=string::npos)path = path.substr(pos);
		
		// Create temporary hdef_t object
		hdef_t hdef(name, path);
		if( (*hist_types)[i].find("TH1")!=string::npos) hdef.type = hdef_t::oneD;
		else if( (*hist_types)[i].find("TH2")!=string::npos) hdef.type = hdef_t::twoD;
		else if( (*hist_types)[i].find("TH3")!=string::npos) hdef.type = hdef_t::threeD;
		else hdef.type = hdef_t::noneD;

		hdef.title = (*hist_titles)[i];
		hdef.active = true;
		
		// Look for entry in RS_INFO
		RS_INFO -> Lock();
		if(RS_INFO->histdefs.find(hdef.hnamepath)==RS_INFO->histdefs.end()){
			RS_INFO->histdefs[hdef.hnamepath]=hdef;
		}else{
			// Need code to verify hdefs ae same!!
		}
		
		// Make sure this server is in list of this hdef's servers
		map<string, bool> &servers = RS_INFO->histdefs[hdef.hnamepath].servers;
		if(servers.find(server)==servers.end())servers[server] = true;

		// Make sure this hdef is in list of this servers hdef's
		vector<string> &hnamepaths = RS_INFO->servers[server].hnamepaths;
		if(find(hnamepaths.begin(), hnamepaths.end(), hdef.hnamepath)==hnamepaths.end())hnamepaths.push_back(hdef.hnamepath);

		RS_INFO -> Unlock();
	}
}

//---------------------------------
// RegisterTreeInfo
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterTreeInfo(string server, cMsgMessage *msg) {

	//TODO: (maybe) test that the msg is valid.
	RS_INFO->Lock();
	vector<tree_info_t> &rs_trees = RS_INFO->servers[server].trees;
	string name = msg->getString("tree_name");
	string path = msg->getString("tree_path");
	vector<string> branch_info = *(msg->getStringVector("branch_info"));
	vector<tree_info_t>::iterator veciter = rs_trees.begin();
	bool duplicate = false;
	for(; veciter != rs_trees.end(); veciter ++) {
		if(veciter->name.compare(name) == 0) duplicate = true;
	}
	if(!duplicate) {
	    // assume that branches are defined at initialization
	    // and don't change during running
	    _DBG_ << "tree info from " << server << " Tree " << name << " in " << path << endl;
	    rs_trees.push_back(tree_info_t(server, name, path, branch_info));
	} 
	RS_INFO->Unlock();

//Test: check RS_INFO for trees
//	RS_INFO->Lock();
//	map<string, server_info_t>::iterator server_iter = RS_INFO->servers.begin();
//	for(; server_iter != RS_INFO->servers.end(); server_iter++) {
//		for(unsigned int i = 0; i< server_iter->second.trees.size(); i++) {
//			cerr<< "path: "<<server_iter->second.trees[i].path << endl;
//			cerr<< "name: "<<server_iter->second.trees[i].name << endl;
//			for(unsigned int j = 0;
//					j < server_iter->second.trees[i].branch_info.size(); j++) {
//				cout<<server_iter->second.trees[i].branch_info[j]<<endl;
//			}
//		}
//	}
//	RS_INFO->Unlock();
}

//---------------------------------
// RegisterMacroList
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterMacroList(string server, cMsgMessage *msg)
{
	/// Copy list of histograms from cMsg into RS_INFO structures.
	bool good_response = true;

	// Get pointers to STL containers that hold the histogram information
	vector<string> *macro_names;
	vector<string> *macro_paths;
	try {                                    //  all args are of type string
		macro_names = msg->getStringVector("macro_names");
		macro_paths = msg->getStringVector("macro_paths");
	} catch (cMsgException e) {
		_DBG_<<"Poorly formed response for \"hists list\". Ignoring."<<endl;
		return;
	}

	// Make sure we have valid pointers
	if(!macro_names)good_response = false;
	if(!macro_paths)good_response = false;

	// Make sure containers all have the same number of entries
	if(good_response){
		if (macro_names->size() != macro_paths->size())good_response = false;
	}

	// If the response is incomplete for any reason, then alert user and return.
	if(!good_response){
		_DBG_<<"Poorly formed response for \"macros list\". Ignoring."<<endl;
		return;
	}

	_DBG_ << "got macro list from " << server << endl;

	// Looks like we got a good response. Loop over histograms and add them to
	// list of hdef_t objects kept in RS_INFO. 
	// store similarly to histograms
	for(unsigned int i=0; i<macro_names->size(); i++){
	
		// Get name
		string name = (*macro_names)[i];

		// Get path without the preceeding root:
		string path = (*macro_paths)[i];

		size_t pos = path.find_first_of("/");
		if(pos!=string::npos) path = path.substr(pos);
		
		// Create temporary hdef_t object
		hdef_t macrodef(name, path);
		macrodef.type = hdef_t::macro;

		//hdef.title = (*hist_titles)[i];
		macrodef.title = "";
		macrodef.active = true;
		
		// Look for entry in RS_INFO
		RS_INFO -> Lock();
		if(RS_INFO->histdefs.find(macrodef.hnamepath)==RS_INFO->histdefs.end()){
			RS_INFO->histdefs[macrodef.hnamepath]=macrodef;
			_DBG_ << "Added macro with hnamepath = " << macrodef.hnamepath << endl;
		}else{
			// Need code to verify hdefs ae same!!
			// for now do some sanity check to at least make sure it is a macro
			if(macrodef.type != RS_INFO->histdefs[macrodef.hnamepath].type) {
				// give up!
				_DBG_ << "Got a macro with hnampath " << macrodef.hnamepath << " that was already defined as a histogram!" << endl
				      << "Exited..." << endl;
				return;
			}
		}
		
		// Make sure this server is in list of this hdef's servers
		map<string, bool> &servers = RS_INFO->histdefs[macrodef.hnamepath].servers;
		if(servers.find(server)==servers.end())
			servers[server] = true;

		// Make sure this hdef is in list of this servers hdef's
		vector<string> &hnamepaths = RS_INFO->servers[server].hnamepaths;
		if(find(hnamepaths.begin(), hnamepaths.end(), macrodef.hnamepath)==hnamepaths.end())
			hnamepaths.push_back(macrodef.hnamepath);

		RS_INFO -> Unlock();
	}
}




//---------------------------------
// RegisterTreeInfo
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterTreeInfoSync(string server, cMsgMessage *msg) {

	//TODO: (maybe) test that the msg is valid.
	RS_INFO->Lock();
	vector<tree_info_t> &rs_trees = RS_INFO->servers[server].trees;

	vector<string> names;
	vector<string> paths;

	try{
		names = *(msg->getStringVector("tree_names"));
		paths = *(msg->getStringVector("tree_paths"));
	}catch(cMsgException e){
		// get here if tree_names or tree_paths doesn't exist
		_DBG_ << "Remote process reports no trees." << endl;
	}

	for( size_t numtree = 0; numtree < names.size(); numtree++) {
	    vector<tree_info_t>::iterator veciter = rs_trees.begin();
	    bool duplicate = false;
	    for(; veciter != rs_trees.end(); veciter ++) {
		if(veciter->name.compare(names[numtree]) == 0) duplicate = true;
	    }
	    if(!duplicate) {
		// assume that branches are defined at initialization
		// and don't change during running
		//_DBG_ << "tree info from " << server << " Tree " << names[numtree] << " in " << paths[numtree] << endl;
		
		// use blank branch info
		vector<string> branch_info;
		rs_trees.push_back(tree_info_t(server, names[numtree], paths[numtree], branch_info));
	    } 
	}

	RS_INFO->Unlock();
}

//---------------------------------
// RegisterTree
//---------------------------------
//TODO: documentation comment.
// Note that we only store tree info on a server-by-server basis,
// so this simplifies the code
void rs_cmsg::RegisterTree(string server, cMsgMessage *msg) 
{

    RS_INFO->Lock();

    string name = msg->getString("tree_name");
    string path = msg->getString("tree_path");

    //_DBG_ << "got the following tree from " << server << ": " << name << " from " << path << endl;

    // Get pointer to server_info_t
    map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
    if(server_info_iter==RS_INFO->servers.end()){
	_DBG_<<"No server_info_t object for server=\""<<server<<"\"!"<<endl;
	_DBG_<<"Throwing away tree."<<endl;
	RS_INFO->Unlock();
	return;
    }
    server_info_t *server_info = &(server_info_iter->second);

    
    // Get pointer to tree_info_t
    tree_info_t *tree_info = NULL;

    vector<tree_info_t> &rs_trees = RS_INFO->servers[server].trees;
    vector<tree_info_t>::iterator treeinfo_iter = rs_trees.begin();
    for(; treeinfo_iter != rs_trees.end(); treeinfo_iter ++) {
	if(treeinfo_iter->name.compare(name) == 0) 
	    break;
    }
    if(treeinfo_iter==rs_trees.end()){
	// tree_info_t object doesn't exist - add it
	vector<string> branch_info = *(msg->getStringVector("branch_info"));
	rs_trees.push_back(tree_info_t(server, name, path, branch_info));

	// get the pointer
	tree_info = &(rs_trees.back());
    } else {
	tree_info = &(*treeinfo_iter);
    }


    /**    
    tree_id_t tree_id(server, name, path);
    vector<tree_info_t>::iterator treeinfo_iter = RS_INFO->trees.find(tree_id_t);
    if(treeinfo_iter==RS_INFO->trees.end()){
	// tree_info_t object doesn't exist. Add one to RS_INFO
	RS_INFO->hinfos[hid] = hinfo_t(server, hnamepath); //???
	hinfo_iter = RS_INFO->hinfos.find(hid); //???
    }
    tree_info = &(*treeinfo_iter);
    **/


    // Get ROOT object from message and cast it as a TNamed*
    pthread_rwlock_wrlock(ROOT_MUTEX);

    _DBG_ << "unpacking tree..." << endl;

    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    Long64_t length;
    TString filename;
    myTM->ReadTString(filename);
    myTM->ReadLong64(length);
    TDirectory *savedir = gDirectory;
    TMemFile *f = new TMemFile(filename, myTM->Buffer() + myTM->Length(), length, "RECREATE");
    savedir->cd();
    
    TNamed *namedObj = NULL;
    
    TIter iter(f->GetListOfKeys());
    TKey *key;
    while( (key = (TKey*)iter()) ) {
	    string objname(key->GetName());
	    
	    cout << "TMemFile object: " << objname << endl;
	    if(objname == name) {
		    TObject *obj = key->ReadObj();
		    TNamed *nobj = dynamic_cast<TNamed*>(obj);
		    if(nobj != NULL) namedObj = nobj;
	    }
    }
    
    if(!namedObj){
	_DBG_<<"No valid object returned in histogram message."<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    
    // Cast this as a tree pointer
    TTree *T = dynamic_cast<TTree*>(namedObj);
    if(!T){
	_DBG_<<"Object received of type \""<<namedObj->ClassName()<<"\" is not a TTree type"<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }

    _DBG_ << "unpacked tree!" << endl;
    T->Print();

    // Update tree_info
    tree_info->received = time(NULL);
    if(tree_info->tree){
	// Delete old histo
	delete tree_info->tree;
	tree_info->tree = NULL;
    }
    // update branch info?

    // Set pointer to hist in hinfo to new histogram
    tree_info->tree = T->CloneTree();
    
    // Change ROOT TDirectory of new histogram to server's
    tree_info->tree->SetDirectory(server_info->dir);

	if(f){
		f->Close();
		delete f;
	}

    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();
}

//---------------------------------
// RegisterHistogram
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterHistogram(string server, cMsgMessage *msg) 
{

    //cerr << "In rs_cmsg::RegisterHistogram()..." << endl;
    
    // Get hnamepath from message
    string hnamepath = msg->getString("hnamepath");
    
    // Lock RS_INFO mutex while working with RS_INFO
    RS_INFO->Lock();
    
    // Get pointer to hdef_t
    map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
    if(hdef_iter==RS_INFO->histdefs.end()){
	_DBG_<<"No hdef_t object for hnamepath=\""<<hnamepath<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	return;
    }
    hdef_t *hdef = &(hdef_iter->second);
    
    // Get pointer to server_info_t
    map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
    if(server_info_iter==RS_INFO->servers.end()){
	_DBG_<<"No server_info_t object for server=\""<<server<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	return;
    }
    server_info_t *server_info = &(server_info_iter->second);
    
    // Get pointer to hinfo_t
    hid_t hid(server, hnamepath);
    hinfo_t *hinfo = NULL;
    map<hid_t,hinfo_t>::iterator hinfo_iter = RS_INFO->hinfos.find(hid);
    if(hinfo_iter==RS_INFO->hinfos.end()){
	// hinfo_t object doesn't exist. Add one to RS_INFO
	RS_INFO->hinfos[hid] = hinfo_t(server, hnamepath);
	hinfo_iter = RS_INFO->hinfos.find(hid);
    }
    hinfo = &(hinfo_iter->second);
    
    // Lock ROOT mutex while working with ROOT objects
    pthread_rwlock_wrlock(ROOT_MUTEX);
    
    // Get ROOT object from message and cast it as a TNamed*
    pthread_rwlock_wrlock(ROOT_MUTEX);
    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    TNamed *namedObj = (TNamed*)myTM->ReadObject(myTM->GetClass());
    if(!namedObj){
	_DBG_<<"No valid object returned in histogram message."<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    
    // Cast this as a histogram pointer
    TH1 *h = dynamic_cast<TH1*>(namedObj);
    if(!h){
	_DBG_<<"Object received of type \""<<namedObj->ClassName()<<"\" is not a TH1 type"<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    
    // Update hinfo
    hinfo->received = time(NULL);
    if(hinfo->hist){
	// Subtract old histo from sum
	if(hdef->sum_hist)hdef->sum_hist->Add(hinfo->hist, -1.0);
	
	// Delete old histo
	delete hinfo->hist;
	hinfo->hist = NULL;
    }

    // Set pointer to hist in hinfo to new histogram
    hinfo->hist = h;
    
    // Change ROOT TDirectory of new histogram to server's
    hinfo->hist->SetDirectory(server_info->dir);
    
    // Adds the new histogram to the hists map in hdef_t
    map<string, hinfo_t>::iterator hinfo_it = hdef->hists.find(server);
    
    // first we have to check for and delete any older versions
    // of the hist.
    if(hinfo_it != hdef->hists.end()) {
	hdef->hists.erase(hinfo_it);
    }
    
    // Now we add the newer version to the map
    hdef->hists.insert(pair<string, hinfo_t>(server, (hinfo_iter->second)));
    
    // Add new histogram to sum and flag it as modified
    _DBG_<<"Adding "<<h->GetEntries()<<" from "<<server<<" to hist "<<hnamepath<<endl;
    if(hdef->sum_hist){
	// Reset sum histo first if showing only one histo at a time
	if(RS_INFO->viewStyle==rs_info::kViewByServer)hdef->sum_hist->Reset();
	hdef->sum_hist->Add(h);
    }else{
      // get the direction in which to save the summed histogram

      //cout << "get sum dir = " << hdef->path.c_str() << endl;
      TDirectory *hist_sum_dir = NULL;
      string sum_path = "/";
      if(hdef->path == "/")
	hist_sum_dir = RS_INFO->sum_dir;
      else {
	sum_path = join(hdef->dirs.begin(), hdef->dirs.end(), "/");
	hist_sum_dir = RS_INFO->sum_dir->GetDirectory(sum_path.c_str());
	//hist_sum_dir = RS_INFO->sum_dir->GetDirectory(hdef->path.c_str());
      }

      cout << "saving in directory " << sum_path << endl;

      // if the directory doesn't exist, make it
      // (needs better error checking)
      if(!hist_sum_dir) {
	hist_sum_dir = RS_INFO->sum_dir->mkdir(sum_path.c_str());
      }
      
      //_DBG_<<"Making summed histogram " << string(h->GetName())+"__sum" << endl;
      string sum_hist_name = string(h->GetName())+"__sum";
      hdef->sum_hist = (TH1*)h->Clone(sum_hist_name.c_str());
      //hdef->sum_hist->SetDirectory(RS_INFO->sum_dir);
      hdef->sum_hist->SetDirectory(hist_sum_dir);
    }

    //would want to update the hdef_t time stamp here (Justin B)
    //_DBG_<<hdef->sum_hist<<endl;
    hdef->sum_hist_modified = true;
    
    //Justin B
    hdef->sum_hist_present = true;

    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();
}



//---------------------------------
// RegisterMacro
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterMacro(string server, cMsgMessage *msg) 
{
	_DBG_ << "In rs_cmsg::RegisterMacro()..." << endl;
    
    // Get hnamepath from message
    string name = msg->getString("macro_name");
    string path = msg->getString("macro_path");
    int version = msg->getInt32("macro_version");
    string hnamepath = path + "/" + name;
    
    // Lock RS_INFO mutex while working with RS_INFO
    RS_INFO->Lock();
    
    // Get pointer to hdef_t
    map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
    if(hdef_iter==RS_INFO->histdefs.end()){
	_DBG_<<"No hdef_t object for hnamepath=\""<<hnamepath<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	return;
    }
    hdef_t *hdef = &(hdef_iter->second);

    // sanity check that this is a macro
    if(hdef->type != hdef_t::macro) {
	    _DBG_ << " Tried to get macro with hnamepath=\"" << hnamepath
		  << "\", but it is already defined as a histogram!" << endl;
	RS_INFO->Unlock();
	return;
    }
    
    // Get pointer to server_info_t
    map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
    if(server_info_iter==RS_INFO->servers.end()){
	_DBG_<<"No server_info_t object for server=\""<<server<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	return;
    }
    server_info_t *server_info = &(server_info_iter->second);
    
    // Get pointer to hinfo_t
    hid_t hid(server, hnamepath);
    hinfo_t *hinfo = NULL;
    map<hid_t,hinfo_t>::iterator hinfo_iter = RS_INFO->hinfos.find(hid);
    if(hinfo_iter==RS_INFO->hinfos.end()){
	// hinfo_t object doesn't exist. Add one to RS_INFO
	RS_INFO->hinfos[hid] = hinfo_t(server, hnamepath);
	hinfo_iter = RS_INFO->hinfos.find(hid);
    }
    hinfo = &(hinfo_iter->second);
    
    // if we already have data for this macro, then throw it away
    // we need to do this before unpacking the new data, since we are reusing the file name
    if(hinfo->macroData) {
	    hinfo->macroData->Close();
	    delete hinfo->macroData;
	    hinfo->macroData = NULL;
    }

    // save version info
    hinfo->macroVersion = version;

    // Lock ROOT mutex while working with ROOT objects
    pthread_rwlock_wrlock(ROOT_MUTEX);
    
    // extract info from message
    _DBG_ << "unpacking macro..." << endl;

    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    Long64_t length;
    myTM->ReadLong64(length);
    TDirectory *savedir = gDirectory;

    // save each macro in a different file per server
    // we'll concatenate these later
    stringstream tmpfile_stream;
    tmpfile_stream << "." << server << "." << hnamepath << endl;
    string tmpfile_name (tmpfile_stream.str());
    for(string::iterator str_itr = tmpfile_name.begin(); str_itr != tmpfile_name.end(); str_itr++) {
	    // clean up filename by making sure that we at least dont have any '/'s
	    if(*str_itr == '/')     
		    *str_itr = '_';
    }
    _DBG_ << " TMemFile name = " << tmpfile_name << endl;
    _DBG_ << "     file size = " << length << endl;
    TString filename(tmpfile_name);

    TMemFile *f = new TMemFile(filename, myTM->Buffer() + myTM->Length(), length);
    //f->ls(); // debugging
    savedir->cd();
    
    hinfo->macroData = f;

    //TNamed *namedObj = NULL;
    /*
    TIter iter(f->GetListOfKeys());
    TKey *key;
    while( (key = (TKey*)iter()) ) {
	    string objname(key->GetName());
	    
	    cout << "TMemFile object: " << objname << endl;
	    if(objname == name) {
		    TObject *obj = key->ReadObj();
		    TNamed *nobj = dynamic_cast<TNamed*>(obj);
		    if(nobj != NULL) {
			    // save objects from the file
		    }
	    }
    }
    */
    /*
    if(!namedObj){
	_DBG_<<"No valid object returned in histogram message."<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
        
    // Cast this as a tree pointer
    TTree *T = dynamic_cast<TTree*>(namedObj);
    if(!T){
	_DBG_<<"Object received of type \""<<namedObj->ClassName()<<"\" is not a TTree type"<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    */
    _DBG_ << "unpacked macro!" << endl;
    //T->Print();
    
    // Update hinfo
    hinfo->received = time(NULL);
    
    // Adds the new histogram to the hists map in hdef_t
    map<string, hinfo_t>::iterator hinfo_it = hdef->hists.find(server);
    
    // first we have to check for and delete any older versions
    // of the hist.
    if(hinfo_it != hdef->hists.end()) {
	hdef->hists.erase(hinfo_it);
    }
    
    // Now we add the newer version to the map
    hdef->hists.insert(pair<string, hinfo_t>(server, (hinfo_iter->second)));
    
    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();
}



//---------------------------------
// RegisterFinalHistogram
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterFinalHistogram(string server, cMsgMessage *msg) {

    // Get hnamepath from message
    string hnamepath = msg->getString("hnamepath");

    // Lock RS_INFO mutex while working with RS_INFO
    RS_INFO->Lock();
    
    // Lock ROOT mutex while working with ROOT objects
    pthread_rwlock_wrlock(ROOT_MUTEX);
    
    // Get ROOT object from message and cast it as a TNamed*
    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    TNamed *namedObj = (TNamed*)myTM->ReadObject(myTM->GetClass());
    if(!namedObj){
	_DBG_<<"No valid object returned in histogram message."<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    
    // Cast this as a histogram pointer
    TH1 *h = dynamic_cast<TH1*>(namedObj);
    if(!h){
	_DBG_<<"Object received of type \""<<namedObj->ClassName()<<"\" is not a TH1 type"<<endl;
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	return;
    }
    
    // save histogram info for consumer thread
    RS_INFO->final_hists.push_back(final_hist_t( server, hnamepath, h ));


    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();
}
