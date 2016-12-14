// $Id$
//
//    File: rs_cmsg.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>
#include <fcntl.h>

#include "RootSpy.h"
#include "rs_cmsg.h"
#include "rs_info.h"
#include "tree_info_t.h"
#include "cMsg.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <set>
using namespace std;

#include <TDirectoryFile.h>
#include <TMessage.h>
#include <TH1.h>
#include <TTree.h>
#include <TMemFile.h>
#include <TKey.h>

double rs_cmsg::start_time=0.0; // overwritten on fist call to rs_cmsg::GetTime()

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
rs_cmsg::rs_cmsg(string &udl, string &name, bool connect_to_cmsg)
{

	verbose = 2; // higher values=more messages. 0=minimal messages
	hist_default_active = true;
	program_name = "rootspy-client"; // should be overwritten by specific program

	udpdev         = NULL;
	udpport        = 0;
	udpthread      = NULL;
	stop_udpthread = false;

	// Connect to cMsg system
	is_online = connect_to_cmsg;
	string myDescr = "Access ROOT objects in a running program";
	cMsgSetDebugLevel(CMSG_DEBUG_WARN); // print most messages
	cMsgSys = new cMsg(udl, name, myDescr);   	// the cMsg system object, where
	try {                         	           	//  all args are of type string
		if(connect_to_cmsg) cMsgSys->connect(); 
	} catch (cMsgException e) {
		cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
		cout << e.toString() << endl; 
		cout<<endl<<endl<<endl<<endl;

		// we need to connect to cMsg to run
		is_online = false;
		exit(0);          
		//return;
	}
	
	if(is_online){
	
		// Create a unique name for ourself
		char hostname[256];
		gethostname(hostname, 256);
		char str[512];
		sprintf(str, "rs_%s_%d", hostname, getpid());
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
	}else{
		cout<<"---------------------------------------------------"<<endl;
		cout<<"  Not connected to cMsg system. Only local histos  "<<endl;
		cout<<"  and macros will be available.                    "<<endl;
		cout<<"---------------------------------------------------"<<endl;	
	}
	
	// Getting list of network devices for direct UDP
	rs_netdevice::GetDeviceList(netdevices);
	rs_netdevice::PrintDevices(netdevices, "Devices available for direct UDP");
	if(!netdevices.empty()){
		// Launch thread to listen for direct UDP packets
		udpthread = new thread(&rs_cmsg::DirectUDPServerThread, this);
	}
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
	
	if(udpthread){
		stop_udpthread = true;
		udpthread->join();
	}
	
	// Optionally write out stats to ROOT file
	const char *ROOTSPY_DEBUG = getenv("ROOTSPY_DEBUG");
	if(ROOTSPY_DEBUG!=NULL){
		string fname = strlen(ROOTSPY_DEBUG)>0 ? ROOTSPY_DEBUG:"rsclient_stats.root";

		// Grab ROOT lock and open output file
		pthread_rwlock_wrlock(ROOT_MUTEX);
		TDirectory *savedir = gDirectory;
		TFile *debug_file = new TFile(fname.c_str(), "RECREATE");
		cout << "--- Writing ROOTSPY_DEBUG to: " << fname << " ----" << endl;

		// Make complete list of all requested and received 
		// hnamepaths. This will allow us to make the x-axis
		// bins of both the requested and received identical
		_DBG_<<"requested_histograms,size()="<<requested_histograms.size()<<endl;
		_DBG_<<"received_histograms,size()="<<received_histograms.size()<<endl;
		set<string> hnamepaths;
		map<string, uint32_t>::iterator iter;
		for(iter=requested_histograms.begin(); iter!=requested_histograms.end(); iter++){
			hnamepaths.insert(iter->first);
			_DBG_<<iter->first<<endl;
		}
		for(iter=received_histograms.begin(); iter!=received_histograms.end(); iter++){
			hnamepaths.insert(iter->first);
			_DBG_<<iter->first<<endl;
		}
		
		if(hnamepaths.empty()) hnamepaths.insert("no_histograms");

		// Requested/received hists
		TH1I * hreqhists = new TH1I("requestedhists", "Requested histograms", hnamepaths.size(), 0, hnamepaths.size());
		TH1I * hrcdhists = new TH1I("receivedhists", "Received histograms", hnamepaths.size(), 0, hnamepaths.size());
		set<string>::iterator siter=hnamepaths.begin();
		for(int ibin=1; siter!=hnamepaths.end(); siter++, ibin++){
			string hnamepath = *siter;
			hreqhists->GetXaxis()->SetBinLabel(ibin, hnamepath.c_str());
			hrcdhists->GetXaxis()->SetBinLabel(ibin, hnamepath.c_str());
			
			hreqhists->SetBinContent(ibin, requested_histograms[hnamepath]);
			hrcdhists->SetBinContent(ibin, received_histograms[hnamepath]);
		}
		
		// Close ROOT file
		debug_file->Write();
		debug_file->Close();
		delete debug_file;

		// Restore ROOT directory and release lock
		savedir->cd();
		pthread_rwlock_unlock(ROOT_MUTEX);

		cout << "-------------- Done --------------" << endl;
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
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
}

//---------------------------------
// BuildRequestHistogram
//---------------------------------
void rs_cmsg::BuildRequestHistogram(cMsgMessage &msg, string servername, string hnamepath)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get hist");
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
    msg.add("hnamepath", hnamepath);
	 if(udpdev){
    	msg.add("udpaddr", udpdev->addr32);
    	msg.add("udpport", udpport);
	}
}

//---------------------------------
// BuildRequestHistograms
//---------------------------------
void rs_cmsg::BuildRequestHistograms(cMsgMessage &msg, string servername, vector<string> &hnamepaths)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get hists");
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
    msg.add("hnamepaths", hnamepaths);
}

//---------------------------------
// BuildRequestTreeInfo
//---------------------------------
void rs_cmsg::BuildRequestTreeInfo(cMsgMessage &msg, string servername)
{
    msg.setSubject(servername);
    msg.setType(myname);
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
    msg.setText("get tree info");
}

//---------------------------------
// BuildRequestTree
//---------------------------------
void rs_cmsg::BuildRequestTree(cMsgMessage &msg, string servername, string tree_name, string tree_path, int64_t num_entries)
{
    msg.setSubject(servername);
    msg.setType(myname);
    msg.setText("get tree");
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
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
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
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
	 uint64_t tsent = (uint64_t)time(NULL);
	 msg.add("time_sent", tsent);
    msg.add("hnamepath", hnamepath);
	 if(verbose>4) _DBG_ << "preparing to request macro with hnamepath=\"" << hnamepath << "\"" << endl;
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
	uint64_t tsent = (uint64_t)time(NULL);
	whosThere.add("time_sent", tsent);
	
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << whosThere.getText() << "\"" << endl;
		cMsgSys->send(&whosThere);
	}
}

//---------------------------------
// RequestHists
//---------------------------------
void rs_cmsg::RequestHists(string servername)
{
	cMsgMessage listHists;
	BuildRequestHists(listHists, servername);	
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << listHists.getText() << "\"" << endl;
		cMsgSys->send(&listHists);
	}
}

//---------------------------------
// RequestHistogram
//---------------------------------
void rs_cmsg::RequestHistogram(string servername, string hnamepath)
{
	cMsgMessage requestHist;
	BuildRequestHistogram(requestHist, servername, hnamepath);
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << "\" for " << hnamepath << endl;
		cMsgSys->send(&requestHist);
		requested_histograms[hnamepath]++;
	}
}

//---------------------------------
// RequestHistograms
//---------------------------------
void rs_cmsg::RequestHistograms(string servername, vector<string> &hnamepaths)
{
	cMsgMessage requestHist;
	BuildRequestHistograms(requestHist, servername, hnamepaths);
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << "\"" << endl;
		cMsgSys->send(&requestHist);
		for(uint32_t i=0; i<hnamepaths.size(); i++) requested_histograms[hnamepaths[i]]++;
	}
}

//---------------------------------
// RequestTreeInfo
//---------------------------------
void rs_cmsg::RequestTreeInfo(string servername)
{
	cMsgMessage treeinfo;
	BuildRequestTreeInfo(treeinfo, servername);
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << treeinfo.getText() << "\"" << endl;
		cMsgSys->send(&treeinfo);
	}
}

//---------------------------------
// RequestTree
//---------------------------------
void rs_cmsg::RequestTree(string servername, string tree_name, string tree_path, int64_t num_entries = -1)
{
	cMsgMessage requestTree;
	BuildRequestTree(requestTree, servername, tree_name, tree_path, num_entries);
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << requestTree.getText() << endl;
		cMsgSys->send(&requestTree);
	}
}

//---------------------------------
// RequestMacroList
//---------------------------------
void rs_cmsg::RequestMacroList(string servername)
{
	cMsgMessage listHists;
	BuildRequestMacroList(listHists, servername);	
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << listHists.getText() << "\"" << endl;
		cMsgSys->send(&listHists);
	}
}

//---------------------------------
// RequestMacro
//---------------------------------
void rs_cmsg::RequestMacro(string servername, string hnamepath)
{
	cMsgMessage requestHist;
	BuildRequestMacro(requestHist, servername, hnamepath);
	if(is_online){
		if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << "\"" << "\"" << endl;
		cMsgSys->send(&requestHist);
		requested_macros[hnamepath]++;
	}
}



//---------------------------------
// RequestHistogramSync
//  Note: Do not lock RS_INFO before calling this function
//---------------------------------
void rs_cmsg::RequestHistsSync(string servername, timespec_t &myTimeout)
{
	if(!is_online) return;

    cMsgMessage requestHist;
    BuildRequestHists(requestHist, servername);
    if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << endl;
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
	if(!is_online) return;

    cMsgMessage requestHist;
    BuildRequestHistogram(requestHist, servername, hnamepath);
    if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << endl;
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
	if(!is_online) return;

    cMsgMessage treeinfo;
    BuildRequestTreeInfo(treeinfo, servername);

    //_DBG_ << " Sending getTreeInfo message..." << endl;
    if(verbose>3) _DBG_ << "Sending \"" << treeinfo.getText() << endl;
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
	if(!is_online) return;

    //_DBG_ << "In rs_cmsg::RequestTreeSync()..." << endl;

    cMsgMessage requestTree;
    BuildRequestTree(requestTree, servername, tree_name, tree_path, num_entries);

    //_DBG_ << " Sending getTree message..." << endl;
    if(verbose>3) _DBG_ << "Sending \"" << requestTree.getText() << endl;
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
	if(!is_online) return;

    cMsgMessage requestHist;
    BuildRequestMacroList(requestHist, servername);
    if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << endl;
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
	if(!is_online) return;

    cMsgMessage requestHist;
    BuildRequestMacro(requestHist, servername, hnamepath);
    if(verbose>3) _DBG_ << "Sending \"" << requestHist.getText() << endl;
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
    
    if(is_online){
    	if(verbose>3) _DBG_ << "Sending \"" << finalhist.getText() << endl;
	 	cMsgSys->send(&finalhist);
	}
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
	if(sender == myname){  // no need to process messages we sent!
		if(verbose>4) cout << "Ignoring message from ourself (\""<<sender<<"\")" << endl;
		delete msg; 
		return;
	}
	
	// Optional Debugging messages
	if(verbose>6){
		map<string,int> *payloads = msg->payloadGet();
		_DBG_ << "cMsg recieved:" << endl;
		if(payloads){
			map<string,int>::iterator iter = payloads->begin();
			for(; iter!=payloads->end(); iter++){
				int payloadtype = msg->payloadGetType(iter->first);
				_DBG_ << "    " << iter->second << " : type=" << payloadtype << " : " << iter->first << endl; 
			}
		}else{
			_DBG_ << "   <no payloads map!>" << endl;
		}
	}

	if(verbose>2){
		// print round trip time
		try{
			uint64_t t_originated = msg->getUint64("time_requester");
			uint64_t t_now = (uint64_t)time(NULL);
			_DBG_ << " " << msg->getText() << " : response time from "<< sender << " = " << (t_now-t_originated) << "secs" << endl;
			if(verbose>4){
				uint64_t t_responded = msg->getUint64("time_sent");
				uint64_t t_received = msg->getUint64("time_received");
				_DBG_ << "  t_originated = " << t_originated << endl; 
				_DBG_ << "    t_received = " << t_received << endl; 
				_DBG_ << "   t_responded = " << t_responded << endl; 
				_DBG_ << "         t_now = " << t_now << endl; 
			}
		}catch(...){}
	}

	if(verbose>4){
		// print round trip time
		try{
			vector<int32_t> *queue_counts = msg->getInt32Vector("queue_counts");
			for(uint32_t i=0; i<queue_counts->size() ;i++){
				_DBG_ << "  queue_counts["<<i<<"] = " << (*queue_counts)[i] << endl;
			}
		}catch(...){}
	}
	
	// Look for an entry for this server in RS_INFO map.
	// If it is not there then add it.
	RS_INFO->Lock();
	
	if(RS_INFO->servers.find(sender)==RS_INFO->servers.end()){
		RS_INFO->servers[sender] = server_info_t(sender);
		if(verbose>=2) cout<<"Added \""<<sender<<"\" to server list."<<endl;
	}else{
		// Update lastHeardFrom time for this server
		RS_INFO->servers[sender].lastHeardFrom = time(NULL);
		if(verbose>=4) _DBG_ <<"Updated \""<<sender<<"\" lastHeardFrom."<<endl;
	}
	RS_INFO->Unlock();
	// The actual command is always sent in the text of the message
	string cmd = msg->getText();
	if(verbose>3) _DBG_ << "msg->getText() = \"" << cmd << "\"" << endl;
	if (cmd == "null"){delete msg; return;}
	// Dispatch command
	bool handled_message = false;
	
	//===========================================================
	if(cmd=="who's there?"){
		// We don't actually respond to these, only servers
		cMsgMessage *response = new cMsgMessage();
		response->setSubject(sender);
		response->setType(myname);
		response->setText("I am here");
		response->add("program", program_name);
		uint64_t tsent = (uint64_t)time(NULL);
		response->add("time_sent", tsent);
		if(verbose>3) _DBG_ << "Sending \"" << response->getText() << endl;
		cMsgSys->send(response);
		delete response;
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
	if(cmd=="histograms"){	
		RegisterHistograms(sender, msg);
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
	// ignore messages sent by other RootSpy gui programs
	if(cmd=="who's there?" ) handled_message = true;
	if(cmd=="list hists"   ) handled_message = true;
	if(cmd=="list macros"  ) handled_message = true;
	if(cmd=="get tree info") handled_message = true;
	if(cmd=="get macro"    ) handled_message = true;
	if(cmd=="get hists"    ) handled_message = true;
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
		if(verbose>2) _DBG_<<"Poorly formed response for \"hists list\". Ignoring."<<endl;
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
		if(verbose>0) _DBG_<<"Poorly formed response for \"hists list\". Ignoring."<<endl;
		return;
	}

	if(verbose>2) _DBG_ << "got histogram list from " << server << endl;

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
		else if( (*hist_types)[i].find("TProfile")!=string::npos) hdef.type = hdef_t::profile;
		else hdef.type = hdef_t::noneD;

		hdef.title = (*hist_titles)[i];
		if(hist_default_active)
			hdef.active = true;
		else
			hdef.active = false;
		
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
	try{
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
	   	 rs_trees.push_back(tree_info_t(server, name, path, branch_info));

			 tree_id_t tid(server, name, path);
			 RS_INFO->treedefs[tid.tnamepath] = tid;
		} 
	}catch(cMsgException &e){
		_DBG_ << "Bad cMsg in RegisterTreeInfo from: " << server << endl;
		_DBG_ << e.what() << endl;
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
		if(verbose>2) _DBG_<<"Poorly formed response for \"macros list\". (May just be empty list. Ignoring.)"<<endl;
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

	if(verbose>=2)_DBG_ << "got macro list from " << server << endl;

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
		if(hist_default_active)
			macrodef.active = true;
		else
			macrodef.active = false;
		
		// Look for entry in RS_INFO
		RS_INFO -> Lock();
		if(RS_INFO->histdefs.find(macrodef.hnamepath)==RS_INFO->histdefs.end()){
			RS_INFO->histdefs[macrodef.hnamepath]=macrodef;
			if(verbose>0)_DBG_ << "Added macro with hnamepath = " << macrodef.hnamepath << endl;
			RequestMacro("rootspy", macrodef.hnamepath);
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

    if(verbose>2) _DBG_ << "got the following tree from " << server << ": " << name << " from " << path << endl;

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
	if(verbose>3){
		cout << "branch_info:" << endl;
		for(uint32_t i=0; i<branch_info.size(); i++){
			cout << "  " << branch_info[i] << endl;
		}
	}

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

    if(verbose>1) _DBG_ << "unpacking tree..." << endl;

    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    Long64_t length;
    TString filename;
    myTM->ReadTString(filename);
    myTM->ReadLong64(length);
    TDirectory *savedir = gDirectory;
    TMemFile *f = new TMemFile(filename, myTM->Buffer() + myTM->Length(), length);
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
	_DBG_<<"No valid object returned in tree message."<<endl;
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

    if(verbose>2)_DBG_ << "unpacked tree!" << endl;
    T->Print();

    // Update tree_info
	double last_received = tree_info->received;
    tree_info->received = GetTime();
	tree_info->rate = 1.0/(tree_info->received - last_received);
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
void rs_cmsg::RegisterHistogram(string server, cMsgMessage *msg, bool delete_msg) 
{
    //cerr << "In rs_cmsg::RegisterHistogram()..." << endl;
    
    // Get hnamepath from message
    string hnamepath = msg->getString("hnamepath");

	  if(verbose>=3) _DBG_ << "Registering histogram " << hnamepath << endl;

    // Lock RS_INFO mutex while working with RS_INFO
    RS_INFO->Lock();
	 
	 received_histograms[hnamepath]++;
    
    // Get pointer to hdef_t
    map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
    if(hdef_iter==RS_INFO->histdefs.end()){
	_DBG_<<"No hdef_t object for hnamepath=\""<<hnamepath<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	if(delete_msg) delete msg;
	return;
    }
    hdef_t *hdef = &(hdef_iter->second);
    
    // Get pointer to server_info_t
    map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
    if(server_info_iter==RS_INFO->servers.end()){
	_DBG_<<"No server_info_t object for server=\""<<server<<"\"!"<<endl;
	_DBG_<<"Throwing away histogram."<<endl;
	RS_INFO->Unlock();
	if(delete_msg) delete msg;
	return;
    }
    server_info_t *server_info = &(server_info_iter->second);
    
    // Get pointer to hinfo_t
    hid_t hid(server, hnamepath);
    map<hid_t,hinfo_t>::iterator hinfo_iter = RS_INFO->hinfos.find(hid);
    if(hinfo_iter==RS_INFO->hinfos.end()){
	// hinfo_t object doesn't exist. Add one to RS_INFO
	RS_INFO->hinfos[hid] = hinfo_t(server, hnamepath);
	hinfo_iter = RS_INFO->hinfos.find(hid);
    }
    hinfo_t *hinfo = &(hinfo_iter->second);
    
    // Lock ROOT mutex while working with ROOT objects
    pthread_rwlock_wrlock(ROOT_MUTEX);
    
    // Get ROOT object from message and cast it as a TNamed*

	 TNamed *namedObj = NULL;
	 vector<uint8_t> *mybytes = NULL;
	 try{
	 	mybytes = msg->getUint8Vector("TMessage");
	 }catch(...){
	   _DBG_ << "Expecting \"TMessage\" payload in histogram message for " << hnamepath << " but none found!" << endl;
	 }
	 if(mybytes){
		MyTMessage *myTM = new MyTMessage(&(*mybytes)[0], mybytes->size());
		namedObj = (TNamed*)myTM->ReadObject(myTM->GetClass());
		if(!namedObj){
			pthread_rwlock_unlock(ROOT_MUTEX);
			RS_INFO->Unlock();
			delete mybytes;
			if(delete_msg) delete msg;
			return;
		}
		delete mybytes;
	}
    
    // Cast this as a histogram pointer
    TH1 *h = dynamic_cast<TH1*>(namedObj);
    if(!h){
	if(namedObj){
		_DBG_<<"Object received of type \""<<namedObj->ClassName()<<"\" is not a TH1 type"<<endl;
	}
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
	if(delete_msg) delete msg;
	return;
    }
    
    // Update hinfo
	double last_received = hinfo->received;
    hinfo->received = GetTime();
	hinfo->rate = 1.0/(hinfo->received - last_received);
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
    if(verbose>=3) _DBG_<<"Adding "<<h->GetEntries()<<" from "<<server<<" to hist "<<hnamepath<<endl;
    if(hdef->sum_hist){
		// Reset sum histo first if showing only one histo at a time
		if(RS_INFO->viewStyle==rs_info::kViewByServer)hdef->sum_hist->Reset();
		hdef->sum_hist->Add(h);
    }else{
      // get the directory in which to save the summed histogram
	  
	  // Make sure the full directory path exists
	  string sum_path = "";
	  TDirectory *hist_sum_dir = RS_INFO->sum_dir;
	  for(uint32_t i=0; i<hdef->dirs.size(); i++){
	  	sum_path += "/" + hdef->dirs[i];
	  	TDirectory *dir = (TDirectory*)(hist_sum_dir->Get(hdef->dirs[i].c_str()));
		if(!dir) dir = hist_sum_dir->mkdir(hdef->dirs[i].c_str());
		hist_sum_dir = dir;
	  }
	  
      if(verbose>2) cout << "saving in directory " << sum_path << endl;

      string sum_hist_name = string(h->GetName())+"__sum";
      hdef->sum_hist = (TH1*)h->Clone(sum_hist_name.c_str());
      //hdef->sum_hist->SetDirectory(RS_INFO->sum_dir);
      hdef->sum_hist->SetDirectory(hist_sum_dir);
	  hdef->sum_hist->SetName(h->GetName()); // set name back to original single hist name so macros work
    }

    // Record time we last modified the sum histo
    hdef->sum_hist_modified = GetTime();
    
    //Justin B
    hdef->sum_hist_present = true;

    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();

	if(delete_msg) delete msg;
}

//---------------------------------
// RegisterHistograms
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterHistograms(string server, cMsgMessage *msg) 
{
	// The cMsgMessage that was sent should contain a vector of other
	// cMgMessage's that are identical to a single histogram's message.
	// Get the vector of these and pass the processing of each to the 
	// single histogram method RegisterHistogram.
	vector<cMsgMessage*> *cmsgs = msg->getMessagePVector("histograms");
	if(!cmsgs){
		_DBG_ << "NULL returned from getMessagePVector() for multiple histograms message!" << endl;
		return;
	}
	if(cmsgs->empty()){
		_DBG_ << "Empty vector (no histograms) for multiple histograms message!" << endl;
		return;
	}

	// Loop over all items and register each
	for(uint32_t i=0; i<cmsgs->size(); i++){
		cMsgMessage *cmsg = (*cmsgs)[i];
		
		RegisterHistogram(server, cmsg);
		
		delete cmsg;
	}
	delete cmsgs;
}

//---------------------------------
// RegisterMacro
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterMacro(string server, cMsgMessage *msg) 
{
	if(verbose>=4) _DBG_ << "In rs_cmsg::RegisterMacro()..." << endl;
    
    // Get hnamepath from message
    string name = msg->getString("macro_name");
    string path = msg->getString("macro_path");
    int version = msg->getInt32("macro_version");
    string hnamepath = path + "/" + name;
    
    // Lock RS_INFO mutex while working with RS_INFO
    RS_INFO->Lock();
    
	 received_macros[hnamepath]++;

    // Get pointer to hdef_t
    map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
    if(hdef_iter==RS_INFO->histdefs.end()){
	_DBG_<<"No hdef_t object for hnamepath=\""<<hnamepath<<"\"!"<<endl;
	_DBG_<<"Throwing away macro."<<endl;
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
	_DBG_<<"Throwing away macro."<<endl;
	RS_INFO->Unlock();
	return;
    }
    
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
    if(verbose>=2) _DBG_ << "unpacking macro \""<<hnamepath<<"\" ..." << endl;

    MyTMessage *myTM = new MyTMessage(msg->getByteArray(),msg->getByteArrayLength());
    Long64_t length;
    myTM->ReadLong64(length);
    TDirectory *savedir = gDirectory;

    // save each macro in a different file per server
    // we'll concatenate these later
    stringstream tmpfile_stream;
    tmpfile_stream << "." << server << "." << hnamepath;
    string tmpfile_name (tmpfile_stream.str());
    for(string::iterator str_itr = tmpfile_name.begin(); str_itr != tmpfile_name.end(); str_itr++) {
	    // clean up filename by making sure that we at least dont have any '/'s
	    if(*str_itr == '/')     
		    *str_itr = '_';
    }
    if(verbose>=4) _DBG_ << " TMemFile name = " << tmpfile_name << endl;
    if(verbose>=4) _DBG_ << "     file size = " << length << endl;
   TString filename(tmpfile_name);

//_DBG_<<"--- Creating TMemFile:" << tmpfile_name <<endl;
    TMemFile *f = new TMemFile(filename, myTM->Buffer() + myTM->Length(), length);
//_DBG_<<"--- Finished" <<endl;
    if(verbose>=4) _DBG_ << "     num. keys = " << f->GetNkeys() << endl;
    if(verbose>4){
	 	_DBG_ << "TMemFile contents: " << endl;
		f->ls();
	 }
    savedir->cd();
    
	TObjString *macro_str = (TObjString *)f->Get("macro");
	if(macro_str) hinfo->macroString = macro_str->GetString().Data();
    hinfo->macroData = f;
	hinfo->Nkeys = f->GetNkeys();
	
	// Look for special comments declaring hnamepaths this
	// macro requires.
	istringstream ss(hinfo->macroString);
	string line;
	uint32_t Nprev_macro_hnamepaths = hdef->macro_hnamepaths.size();
	hdef->macro_hnamepaths.clear();
	while(getline(ss, line)){
		if(line.find("// hnamepath")==0){
			string myhnamepath = line.substr(12);
			myhnamepath.erase(myhnamepath.find_last_not_of(" \n\r\t")+1);
			hdef->macro_hnamepaths.insert(myhnamepath);
			if(verbose>1) _DBG_<<"Added " <<  myhnamepath << " to macro: " << hnamepath << endl;
		}
	}

	// If we have a different number of macro input histograms
	// than before then spin off a thread to start asking for
	// them. This should usually happen only at start up when
	// a macro is read in for the first time. This helps by 
	// pre-loading histograms that will be needed for display.
	if(hdef->macro_hnamepaths.size() != Nprev_macro_hnamepaths){
_DBG_<<"seeding ... hdef->macro_hnamepaths.size()="<<hdef->macro_hnamepaths.size()<<" Nprev_macro_hnamepaths="<<Nprev_macro_hnamepaths<<endl;
		thread t(&rs_cmsg::SeedHnamepathsSet, this, (void*)&hdef->macro_hnamepaths, true, false);
		t.detach();
	}

    // Update hinfo
	double last_received = hinfo->received;
    hinfo->received = GetTime();
 	hinfo->rate = 1.0/(hinfo->received - last_received);
   
    // Adds the new histogram to the hists map in hdef_t
    map<string, hinfo_t>::iterator hinfo_it = hdef->hists.find(server);
    
    // first we have to check for and delete any older versions
    // of the hist.
    if(hinfo_it != hdef->hists.end()) {
	hdef->hists.erase(hinfo_it);
    }
    
    // Now we add the newer version to the map
    hdef->hists.insert(pair<string, hinfo_t>(server, (hinfo_iter->second)));

    // Record time we last modified the sum histo
    hdef->sum_hist_modified = GetTime();
    
    // Unlock mutexes
    pthread_rwlock_unlock(ROOT_MUTEX);
    RS_INFO->Unlock();
}



//---------------------------------
// RegisterFinalHistogram
//---------------------------------
//TODO: documentation comment.
void rs_cmsg::RegisterFinalHistogram(string server, cMsgMessage *msg) {

    // first, save the histogram in memory
    RegisterHistogram(server, msg);

    // then, store the histogram in a list for optional processing by the client program
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

//---------------------------------
// SeedHnamepathsSet
//---------------------------------
void rs_cmsg::SeedHnamepathsSet(void *vhnamepaths, bool request_histo, bool request_macro)
{
	set<string> &hnamepaths = *(set<string>*)vhnamepaths;

	/// This is just a wrapper for the "list" version below.
	list<string> lhnamepaths;
	set<string>::iterator iter = hnamepaths.begin();
	for(; iter!=hnamepaths.end(); iter++) lhnamepaths.push_back(*iter);
	
	SeedHnamepaths(lhnamepaths, request_histo, request_macro);
}

//---------------------------------
// SeedHnamepaths
//---------------------------------
void rs_cmsg::SeedHnamepaths(list<string> &hnamepaths, bool request_histo, bool request_macro)
{
	/// This is run in a separate, temporary thread
	/// to request the histograms and/or macros with
	/// the given hnamepaths. This is called either
	/// from ReadConfig as a new RSTab's configuration
	/// is read in, or from RegisterMacro when a macro
	/// is first read that specifies a set of hnamepaths
	/// it needs. The intent is for them to be slowly read 
	/// in the background when the program first starts
	/// up so that by the time the user asks for them,
	/// they may already be here making the program
	/// display them faster.
	///
	/// This loops over the given hnamepaths requesting
	/// each from the generic "rootspy" since we don't
	/// yet know what producers are there. It may also
	/// requests the hnamepath as both a histogram and
	/// a macro (depending on the supplied flags) since
	/// we may not actually don't know at this point which
	/// it is!
	///
	/// In order to not overwhelm the remote
	/// processes, requests are sent out one at a time
	/// with a sleep call in between. Thus, this may
	/// take a while to send out all requests.

	// If requesting histos, a single request may be sent for
	// all of them.
	if(request_histo){
		vector<string> shnamepaths;
		list<string>::iterator iter = hnamepaths.begin();
		for(; iter!=hnamepaths.end(); iter++)shnamepaths.push_back(*iter);
		
		if(verbose>0) _DBG_ << "SeedHnamepaths: requesting " << shnamepaths.size() << " histos" << endl;
		RequestHistograms("rootspy", shnamepaths);
	}

	if(request_macro){
		list<string>::iterator iter = hnamepaths.begin();
		for(; iter!=hnamepaths.end(); iter++){
			string hnamepath = *iter;
		
			if(verbose>0) _DBG_ << "SeedHnamepaths: requesting macro " << hnamepath << endl;
			RequestMacro("rootspy", hnamepath);
			//usleep(10000);
		}
	}
}

//---------------------------------
// DirectUDPServerThread
//---------------------------------
void rs_cmsg::DirectUDPServerThread(void)
{
	/// This method is run in a thread to listen for
	/// UDP packets sent here directly from a remote
	/// histogram producer. This allows the bulky
	/// histogram messages to be sent this way instead
	/// of through the cMsg server.

	if(netdevices.empty()) return; // extra bomb-proofing

	rs_netdevice *dev = netdevices[0];
	
	// Create socket
	int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(fd<0){
		perror("unable to create socket for UDP direct!");
		return;
	}
	
	// Make socket non-blocking
	if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0 ){
		perror("can't set socket to non-blocking!");
		return;
	}
	
	// Bind to address and have system assign port number
	struct sockaddr_in myaddr;
	memset((char*)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY); // accept packets from any device
	myaddr.sin_port = 0; // let system assign a port
	if( ::bind(fd, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0){
		perror("bind failed");
		close(fd);
		return;
	}
	
	// Get port number
	socklen_t socklen = sizeof(myaddr);
	if(getsockname(fd, (struct sockaddr*)&myaddr, &socklen) < 0){
		perror("getsockname failed");
		close(fd);
		return;
	}
	udpport = myaddr.sin_port;

	cout << "Launched UDP server using " << dev->name << " - " << dev->ip_dotted << " : " << udpport << endl;
	
	// upddev is used as flag to indicate we're able to
	// receive histograms this way. If we don't get here,
	// it will be left as NULL and the direct UDP feature
	// will not be used.
	udpdev = dev;
	
	uint32_t buff_size = 10000000;
	uint8_t *buff = new uint8_t[buff_size];
	
	while(!stop_udpthread){
	
		// Try and read some data
		socklen = sizeof(myaddr);
		ssize_t bytes_recvd = recvfrom(fd, buff, buff_size, 0, (struct sockaddr*)&myaddr, &socklen);
		if(bytes_recvd < 0){
			// Seems dangerous to check errno here due to
			// there being multiple threads ....
			if( (errno==EAGAIN) || (errno==EWOULDBLOCK) ){
				// no data. sleep for 10ms
				usleep(10000);
				continue;
			}else{
				perror("receiving on rootspy direct UDP");
				break;
			}
		}
		
		if(verbose>4) _DBG_ << "received UDP with " << bytes_recvd << " bytes!" << endl;
		
		rs_udpmessage *rsudp = (rs_udpmessage*)buff;
		string mtype = "unknown";
		switch(rsudp->mess_type){
			case kUDP_HISTOGRAM: mtype="histogram"; break;
			case kUDP_MACRO    : mtype="macro";     break;
		}
		string hnamepath = (const char*)rsudp->hnamepath;
		string sender = (const char*)rsudp->sender;
		uint8_t *buff8 = (uint8_t*)&rsudp->buff_start;
		if(verbose>2) _DBG_ << "UDP: hnamepath=" << rsudp->hnamepath << " type=" << mtype << " from " << sender << endl;

		// Write info into a cMsg object so we can
		// let the same RegisterHistogram method handle
		// it as handles histograms coming from cMsg itself.
		cMsgMessage *cmsg = new cMsgMessage();
		cmsg->setSubject(myname);
		cmsg->setType(sender);
		cmsg->setText("histogram");
		cmsg->add("hnamepath", hnamepath);
		cmsg->add("TMessage", buff8, rsudp->buff_len);
		cmsg->add("time_sent", rsudp->time_sent);
		cmsg->add("time_requester", rsudp->time_requester);
		cmsg->add("time_received", rsudp->time_received);

		// Launch separate thread to handle this
		thread t(&rs_cmsg::RegisterHistogram, this, sender, cmsg, true);
		t.detach();
		
	}
	
	close(fd);
	delete[] buff;
}



