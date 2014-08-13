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
#include <unistd.h>

#include <iostream>
#include <set>
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
bool    InitRootSpy(void);
bool    rsHelpShort(void);
void    rsHelp(void);
void    rsList(void);
TH1*    rsGet(const char *hnamepath);
TTree*  rsGetTree(const char *name, const char *path, unsigned long Nentries, const char *server);
};

bool nada = rsHelpShort(); // Force printing of message at plugin load


//--------------------
// InitRootSpy
//--------------------
bool InitRootSpy(void)
{
	cout << "Attempting to connect to cMsg server ..." << endl;

	// Get ROOTSPY_UDL from environment
	const char *ptr = getenv("ROOTSPY_UDL");
	if(ptr)ROOTSPY_UDL = ptr;

	// Create cMsg name from host and pid
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "RScint plugin %s-%d", hostname, getpid());
	CMSG_NAME = string(str);

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
	cout << "	TH1* Get(hnamepath)" << endl;
	cout << "	TTree* GetTree(name, path, Nentries=0, server=\"\")" << endl;
	cout << "" << endl;
	cout << "where: hnamepath, name, path, and server are all const char*" << endl;
	cout << "types. Nentries is an unsigned long that defaults to 0" << endl;
	cout << "meaning get all entries. If the server is not specified for" << endl;
	cout << "GetTree, then the first server in the list of active servers " << endl;
	cout << "that has the specified name and path will be used." << endl;
	cout << "" << endl;
	cout << "Use the List() method to get a list of available" << endl;
	cout << "histograms and trees from the remote servers. Note that" << endl;
	cout << "RootSpy supports the full ROOT directory structure" << endl;
	cout << "so names may include some path information at the" << endl;
	cout << "beginning. TTrees are handled slightly differently" << endl;
	cout << "sp the name and path must be specified separately." << endl;
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
	cout << "  root [5] TTree *T = rs->GetTree(\"T\", \"root:/\")" << endl;
	cout << "  root [6] T->Draw(\"E\")" << endl;
	cout << "" << endl;


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
		RS_CMSG->RequestTreeInfo(server);
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
			case hdef_t::macro:   type = "macro";  break;
		}
		
		string &hname = hdef.hnamepath;
		cout << " " << type << "  " << hname << string(max_name_len-hname.length()+2, ' ');
		cout << hdef.title << endl;
	}
	
	// Trees are stored such that we only have a list of
	// trees for each server and not a single union list.
	// Create the union list here.
	set<pair<string,string> > treenames;
	max_name_len = 4; // for "name" in title
	for(iter=servers.begin(); iter!=servers.end(); iter++){
		vector<tree_info_t> &trees = iter->second.trees;
		for(unsigned int i=0; i<trees.size(); i++){
			string &name = trees[i].name;
			string &path = trees[i].path;
			treenames.insert(pair<string,string>(name, path));
			int len = name.length();
			if(len > max_name_len) max_name_len = len;
		}
	}
	
	// If no trees are defined then just return now
	if(treenames.size() == 0) return;

	
	// Print list of trees
	cout << endl;
	cout << "TTrees:" << endl;
	cout << "name" << string(max_name_len+2-4, ' ') << "  path" << endl;
	cout << "----" << string(max_name_len+2-4, '-') << " ---------" << endl;
	set<pair<string,string> >::iterator titer;
	for(titer=treenames.begin(); titer!=treenames.end(); titer++){
		string name = (*titer).first;
		string path = (*titer).second;
		cout << " " << name << string(max_name_len-name.length()+2, ' ') << path << endl;
	}
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
// rsGetTree
//--------------------
TTree*  rsGetTree(const char *name_c, const char *path_c, unsigned long Nentries, const char *server_c)
{
	if(!RS_CMSG) return NULL;
	RS_CMSG->PingServers();

	map<string,server_info_t> &servers = RS_INFO->servers;
	map<string,server_info_t>::iterator iter;
	
	string name(name_c);
	string path(path_c);
	string server(server_c);

	if(server.length() == 0){
		// if server is not specified, use first in list that
		// has this tree.
		for(iter=servers.begin(); iter!=servers.end(); iter++){
			vector<tree_info_t> &trees = iter->second.trees;
			for(unsigned int i=0; i<trees.size(); i++){
				if( (trees[i].name==name) && (trees[i].path==path) ){
					server = iter->first;
					break;
				}
			}
			if(server.length()!=0) break;
		}
	}
	
	if(server.length() == 0){
		cout << "-- No servers have the specified name and path --" << endl;
		return NULL;
	}
	
	cout << "Requesting tree name:" << name << " path:" << path << " from " << server << endl;
	timespec_t timeout = {5, 0L};
	RS_CMSG->RequestTreeSync(server, name, path, timeout, Nentries);
	
	// Look through server/tree list again to get pointer to tree
	for(iter=servers.begin(); iter!=servers.end(); iter++){
		if(iter->first != server) continue;
		vector<tree_info_t> &trees = iter->second.trees;
		for(unsigned int i=0; i<trees.size(); i++){
			if( (trees[i].name==name) && (trees[i].path==path) ){
				return trees[i].tree;
			}
		}
	}

	cout << "-- No TTree returned! --" << endl;
	return NULL;
}

