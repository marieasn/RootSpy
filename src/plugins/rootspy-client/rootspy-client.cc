//
//
//  This file is used to create a plugin that can
// be loaded into a running ROOT session in order
// to obtain ROOT objects via RootSpy. Use it like
// this:
//
//  0.) Set your ROOTSPY_UDL environment variable
//      if it is not set already. (You can try
//      something like "cMsg://gluondb1/cMsg/rootspy"
//      if in the Hall-D counting house.)
//
//  1.) Locate the rootspy-client.so file. This is
//      installed in the plugins directory which
//      lives beside bin, lib, and include
//
//  2.) Fire up root and load the library:
//
//      root [0] gSystem.Load("path/to/rootspy-client.so")
//
//
//  3.) Create a RScint object for interacting
//      with the RootSpy system
//
//      root [1] RScint *rs = new RScint()
//
//
//  4.) Get a remote histogram
//
//      root [2] TH1 *myhist = (TH1*)rs->Get("myhist")
//
//
//  5.) Plot the histogram
//
//      root [3] myhist->Draw();
//

#include <iostream>
using namespace std;

#include <rs_cmsg.h>
#include <rs_info.h>

#include "RScint.h"


rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;

string ROOTSPY_UDL = "cMsg://gluondb1/cMsg/rootspy";
string CMSG_NAME = "rootspy_test";


extern "C" {
bool InitRootSpy(void);
bool rsHelpShort(void);
void rsHelp(void);
TH1* rsGet(const char *hnamepath);
void rsList(void);
};

bool nada = rsHelpShort(); // Force printing of message at plugin load


//--------------------
// InitRootSpy
//--------------------
bool InitRootSpy(void)
{
	cout << "Attempting to connect to cMsg server ..." << endl;

	RS_INFO = new rs_info();
	RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);
	
	if(RS_CMSG) RS_CMSG->PingServers();
	
	return true;
}

//--------------------
// rsHelpShort
//--------------------
bool rsHelpShort(void)
{
	cout << endl;
	cout << "---- rootspy-client.so plugin loaded ----" << endl;
	cout << " (type RScint::Help() for detailed help)" << endl;
	cout << "-----------------------------------------" << endl;
	cout << endl;

	// return a value so we can use this routine to
	// initialize a global variable, causing it to
	// get called as soon as the plugin is loaded
	return true; 
}

//--------------------
// rsHelp
//--------------------
void rsHelp(void)
{
	cout << "" << endl;
	cout << "RootSpy interactive plugin (rootspy-client.so)" << endl;
	cout << "" << endl;
	cout << "" << endl;
	cout << "This plugin provides access to the RootSpy histogram" << endl;
	cout << "system while in an interactive root session. This allows" << endl;
	cout << "you to probe remote histogram producing programs and" << endl;
	cout << "download copies of their histograms while they are being" << endl;
	cout << "filled. If multiple nodes provide the same type of" << endl;
	cout << "histogram (e.g. you are running monitoring on a farm)," << endl;
	cout << "then the histograms from all nodes will be retrieved and" << endl;
	cout << "summed automatically." << endl;
	cout << "" << endl;
	cout << "To use this, you'll need to have your ROOTSPY_UDL" << endl;
	cout << "environment variable set prior to starting your ROOT" << endl;
	cout << "session. This will be something like:" << endl;
	cout << "" << endl;
	cout << "   setenv ROOTSPY_UDL cMsg://gluondb1/cMsg/rootspy" << endl;
	cout << "" << endl;
	cout << "Once in the root session, you'll need to load the" << endl;
	cout << "rootspy-client plugin. This is probably located at" << endl;
	cout << "$ROOTSPY/plugins/rootspy-client.so. Load it like" << endl;
	cout << "this:" << endl;
	cout << "" << endl;
	cout << "  root [0] gSystem.Load(\"path/to/rootspy-client.so\")" << endl;
	cout << "" << endl;
	cout << "Once it is loaded, you can create a RScint object" << endl;
	cout << "which is used to access the system:" << endl;
	cout << "" << endl;
	cout << "  root [1] RScint *rs = new RScint()" << endl;
	cout << "" << endl;
	cout << "Available methods of RScint are:" << endl;
	cout << "" << endl;
	cout << "	Help()" << endl;
	cout << "	List()" << endl;
	cout << "	TH1* Get(const char *hnamepath)" << endl;
	cout << "" << endl;
	cout << "Use the List() method to get a list of available" << endl;
	cout << "histograms from the remote servers. Note that" << endl;
	cout << "RootSpy supports the full ROOT directory structure" << endl;
	cout << "so names may include some path information at" << endl;
	cout << "the beginning." << endl;
	cout << "" << endl;
	cout << "  root [2] rs->List()" << endl;
	cout << "  Requesting histogram names from all servers ..." << endl;
	cout << "  waiting for all servers to respond ..." << endl;
	cout << "  Added \"gluon30.jlab.org_28853\" to server list." << endl;
	cout << "  type name              title" << endl;
	cout << "  ---- ---------------- ---------" << endl;
	cout << "   1D  //E             Energy" << endl;
	cout << "   1D  //Mass          Mass" << endl;
	cout << "   1D  /components/px  Momentum X-component" << endl;
	cout << "   1D  /components/py  Momentum Y-component" << endl;
	cout << "   1D  /components/pz  Momentum Z-component" << endl;
	cout << "" << endl;
	cout << "To get a histogram from the remote server(s) use" << endl;
	cout << "the Get() method, passing the fully-qualified" << endl;
	cout << "name as shown by List()." << endl;
	cout << "" << endl;
	cout << "  root [3] TH1 *mass = rs->Get(\"//Mass\")" << endl;
	cout << "  root [4] mass->Draw()" << endl;
	cout << "" << endl;
	cout << "example session (abridged):" << endl;
	cout << "---------------------------------" << endl;
	cout << "  root [0] gSystem.Load(\"path/to/rootspy-client.so\")" << endl;
	cout << "  root [1] RScint *rs = new RScint()" << endl;
	cout << "  root [2] rs->List()" << endl;
	cout << "  root [3] TH1 *mass = rs->Get(\"//Mass\")" << endl;
	cout << "  root [4] mass->Fit(\"gaus\")" << endl;
	cout << "" << endl;


}

//--------------------
// rsGet
//--------------------
TH1* rsGet(const char *hnamepath)
{
	if(!RS_CMSG) return NULL;
	RS_CMSG->PingServers();

	map<string,server_info_t> &servers = RS_INFO->servers;
	map<string,server_info_t>::iterator iter;

	// Loop over all servers, requesting the histogram from each
	for(iter=servers.begin(); iter!=servers.end(); iter++){

		string server = iter->first;
		cout << "Requesting histogram: " << hnamepath << " from " << server << endl;
		RS_CMSG->RequestHistogram(server, string(hnamepath));
	}
	
	// Wait a second for the servers to respond
	cout << "waiting for all servers to respond ..." << endl;
	sleep(1);
	
	// Get the summed histogram
	if(RS_INFO->histdefs.find(hnamepath) != RS_INFO->histdefs.end()){
		hdef_t &hdef = RS_INFO->histdefs[hnamepath];
		return hdef.sum_hist;
	}
	
	// No histogram with the given namepath returned. Inform user
	cout << "-- No histogram with that name --" << endl;
	
	return NULL;
}

//--------------------
// rsList
//--------------------
void rsList(void)
{
	if(!RS_CMSG) return;
	RS_CMSG->PingServers();

	map<string,server_info_t> &servers = RS_INFO->servers;
	map<string,server_info_t>::iterator iter;

	// Loop over all servers, requesting the list of histograms from each
	cout << "Requesting histogram names from all servers ..." << endl;
	for(iter=servers.begin(); iter!=servers.end(); iter++){
	
		string server = iter->first;
		RS_CMSG->RequestHists(server);
	}

	// Wait a second for the servers to respond
	cout << "waiting for all servers to respond ..." << endl;
	sleep(1);
	
	// Get list of histograms
	map<string,hdef_t> &histdefs = RS_INFO->histdefs;
	map<string,hdef_t>::iterator hiter;
	
	// Get longest name so we can display pretty formatting
	int max_name_len = 4; // for "name" in title
	for(hiter=histdefs.begin(); hiter!=histdefs.end(); hiter++){
		int len = hiter->second.hnamepath.length();
		if(len > max_name_len) max_name_len = len;
	}
	
	// Print all histograms
	cout << "type name" << string(max_name_len+2-4, ' ') << "  title" << endl;
	cout << "---- ----" << string(max_name_len+2-4, '-') << " ---------" << endl;
	for(hiter=histdefs.begin(); hiter!=histdefs.end(); hiter++){
		hdef_t &hdef = hiter->second;
		string type = "??";
		switch(hdef.type){
			case hdef_t::noneD:   type = "0D";  break;
			case hdef_t::oneD:    type = "1D";  break;
			case hdef_t::twoD:    type = "2D";  break;
			case hdef_t::threeD:  type = "3D";  break;
		}
		
		string &hname = hdef.hnamepath;
		cout << " " << type << "  " << hname << string(max_name_len-hname.length()+2, ' ');
		cout << hdef.title << endl;
	}
}


