
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <TFile.h>
#include <TDirectory.h>
#include <TObjArray.h>
#include <TH1F.h>
#include <TClass.h>
#include <TTree.h> 
#include <TList.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "RSDumpTree.h"

#include "rs_cmsg.h"
#include "rs_info.h"


// GLOBALS
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;


// configuration variables
namespace config {
    static string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
    static string CMSG_NAME = "<not set here. see below>";
    static string SESSION = "";
    
    static string OUTPUT_FILENAME = "output.root";
    static string IN_TREE_NAMES = "";
    
    static double POLL_DELAY = 5;            // delay for initial discovery of servers  (in sec)
    static long NUM_REQUESTED_EVENTS = -1;
    //static timespec_t SYNC_TIMEOUT = {30,0};  // timeout for synchronous requests  (in sec)
    static timespec_t SYNC_TIMEOUT = {10,0};  // timeout for synchronous requests
}

using namespace config;

// ---------------------------------------------------------------------------------


void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);
void SaveTrees( TFile *the_file );

bool IsGoodTFile( TFile *the_file )
{
    return ((the_file!=NULL) && !(the_file->IsZombie()));
}

//-------------------
// main
//-------------------
int main(int narg, char *argv[])
{
    // Parse the command line arguments
    ParseCommandLineArguments(narg, argv);

    // figure out which trees we're supposed to get
    if(IN_TREE_NAMES == "") {
	cout << " No trees specified to dump!" << endl << endl;
	Usage();
	return 0;
    }

    vector<string> tree_names;
    const string kDelimiter = ",";
    size_t current;
    size_t next = -1;
    do
    {
	current = next + 1;
	next = IN_TREE_NAMES.find_first_of( kDelimiter, current );
	tree_names.push_back( IN_TREE_NAMES.substr( current, next - current ) );
    } while (next != string::npos);

    if(tree_names.size() == 0) {
	cout << " Could not find any tree names!" << endl << endl;
	return 0;
    }

    cout << endl;
    cout << "--------------------------------------" << endl;
    cout << "TTrees to save:" << endl;
    for(vector<string>::const_iterator str_itr = tree_names.begin();
	str_itr != tree_names.end(); str_itr++) {
	cout << "   " << *str_itr << endl;
    }
    cout << endl << endl;

    //exit(0);

    // make file to store thre trees in
    TFile* output_file = new TFile(OUTPUT_FILENAME.c_str(), "recreate"); 
    if(!IsGoodTFile(output_file)) {
	cout << "Couldn't make output file, exiting" << endl;
	return 0;
    }

    // Makes a Mutex to lock / unlock Root Global
    ROOT_MUTEX = new pthread_rwlock_t;
    pthread_rwlock_init(ROOT_MUTEX, NULL);
        
    // Initialize RootSpy information
    RS_INFO = new rs_info();

    // Create cMsg object
    char hostname[256];
    gethostname(hostname, 256);
    char str[512];
    sprintf(str, "RSDumpTree");
    CMSG_NAME = string(str);
    cout << "Full UDL is " << ROOTSPY_UDL << endl;
    RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);

    // sleep for a bit while we figure out which servers are out there
    sleep(POLL_DELAY);

    // figure out who has what trees
    //RS_INFO->Lock();
    for(map<string,server_info_t>::const_iterator server_it = RS_INFO->servers.begin();
	server_it != RS_INFO->servers.end(); server_it++) {
	RS_CMSG->RequestTreeInfoSync(server_it->first, SYNC_TIMEOUT);
	//RS_CMSG->RequestTreeInfo(server_it->first);
    }
    //RS_INFO->Unlock();

    // wait for the requests to finish up
    //sleep(POLL_DELAY);

    // collect trees 
    // the handler that receives the tree that we request already locks RS_INFO, 
    // so don't lock it here for now... this isn't the most thread-safe way, 
    // though maybe it doesn't matter too much?
    //RS_INFO->Lock();
    for( map<string,server_info_t>::iterator server_itr = RS_INFO->servers.begin();
	 server_itr!=RS_INFO->servers.end(); server_itr++) { 
	_DBG_ << "checking " << server_itr->first << "..." << endl;

	for( vector<tree_info_t>::iterator tree_itr = server_itr->second.trees.begin();
	     tree_itr != server_itr->second.trees.end(); tree_itr++ ) {
	    _DBG_ << tree_itr->tnamepath << endl;
	    
	    if(find(tree_names.begin(), tree_names.end(), tree_itr->name) != tree_names.end()) {
		RS_CMSG->RequestTreeSync( server_itr->first, tree_itr->name, tree_itr->path, 
					  SYNC_TIMEOUT, NUM_REQUESTED_EVENTS );
	    }
	}
    }
    //RS_INFO->Unlock();

    // for now do the simplest thing, and just wait for the responses to roll in
    //sleep(POLL_DELAY);

    // get the trees we want and save them
    pthread_rwlock_wrlock(ROOT_MUTEX);
    SaveTrees(output_file);
    output_file->Close();
    pthread_rwlock_unlock(ROOT_MUTEX);


    // finish up
    delete RS_CMSG;
    //delete RS_INFO;
    
    return 0;
}

//-----------
// ParseCommandLineArguments
// configuration priorities (low to high):
//    config file -> environmental variables -> command line
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
  _DBG_ << "In ParseCommandLineArguments().." << endl;


  // allow for environmental variables
  const char *ptr = getenv("ROOTSPY_UDL");
  if(ptr)ROOTSPY_UDL = ptr;
  
  // check command line options
  static struct option long_options[] = {
        {"help",           no_argument,       0,  'h' },
        {"poll-delay",     required_argument, 0,  'p' },
        {"udl",            required_argument, 0,  'u' },
        {"server",         required_argument, 0,  's' },
        {"output-file",    required_argument, 0,  'F' },
        {"tree-names",     required_argument, 0,  'T' },
	{"num-events",     required_argument, 0,  'n' },
	{"sync-timeout",   required_argument, 0,  'c' },

        {0, 0, 0, 0  }
    };

  int opt = 0;
  int long_index = 0;
  while ((opt = getopt_long(narg, argv,"hpusFT", 
			    long_options, &long_index )) != -1) {
    switch (opt) {
    case 'p' : 
      if(optarg == NULL) Usage();
      POLL_DELAY = atoi(optarg);
      break;
    case 'u' :
      if(optarg == NULL) Usage();
      ROOTSPY_UDL = optarg;
      break;
    case 'T' :
      if(optarg == NULL) Usage();
      IN_TREE_NAMES = optarg;
      break;
    case 's' :
      if(optarg == NULL) Usage();
      ROOTSPY_UDL = "cMsg://";
      ROOTSPY_UDL += optarg;
      ROOTSPY_UDL += "/cMsg/rootspy";
      break;
    case 'F' :
      if(optarg == NULL) Usage();
      OUTPUT_FILENAME = optarg;
      break;
    case 'n':
	if(optarg == NULL) Usage();
	NUM_REQUESTED_EVENTS = atoi(optarg);
	break;
    case 'c':
	if(optarg == NULL) Usage();
	SYNC_TIMEOUT.tv_sec = atoi(optarg);
    case 'h':
    default: Usage(); break;
    }
  }  
  
}



//-----------
// Usage
//-----------
void Usage(void)
{
    cout<<"Usage:"<<endl;
    cout<<"       RSDumpTree [options]"<<endl;
    cout<<endl;
    cout<<"Saves remote TTree(s) in ROOT file." << endl;
    cout<<endl;
    cout<<"Options:"<<endl;
    cout<<endl;
    cout<<"   -h,--help                 Print this message"<<endl;
    cout<<"   -T,--tree-names trees     Names of TTrees to read out"<<endl;
    cout<<"   -F,--output-file file     Name of ROOT file used to store output"<<endl; 
    cout<<"   -n,--num-events N         Save only the latest N events of the TTree(s)"<<endl;
    cout<<"   -u,--udl udl              UDL of cMsg RootSpy server"<<endl;
    cout<<"   -s,--server server-name   Set RootSpy UDL to point to server IP/hostname"<<endl;
    cout<<"   -p,--poll-delay time      Time (in seconds) to wait between polling servers" << endl;
    cout<<"   -c,--sync-timeout         Time (in seconds) to wait when requesting TTree data" << endl;
    cout<<endl;
    
    exit(0);
}


// assumes calling function locks the mutex
void SaveTrees( TFile *the_file )
{
    _DBG_ << "In SaveTrees()..." << endl;

    // we have to keep TLists of the various trees so that we can merge them in the end.
    // the key is the full path of the tree
    map< string, TList* > tree_lists;

    // for each server, loop through their trees and collect them
    for( map<string,server_info_t>::iterator server_itr = RS_INFO->servers.begin();
	 server_itr!=RS_INFO->servers.end(); server_itr++) { 

	_DBG_ << "checking " << server_itr->first << "..." << endl;

	for( vector<tree_info_t>::iterator tree_itr = server_itr->second.trees.begin();
	     tree_itr != server_itr->second.trees.end(); tree_itr++ ) {
	    _DBG_ << tree_itr->tnamepath << endl;

	    if(tree_itr->tree) {
		if(tree_lists.find( tree_itr->tnamepath ) == tree_lists.end() )
		    tree_lists[ tree_itr->tnamepath ] = new TList();
	    	tree_lists[ tree_itr->tnamepath ]->Add( tree_itr->tree );
	    }
	}
    }

    _DBG_ << "merging trees" << endl;

    // now merge all the trees into new trees and put them in the proper place in the file
    for(  map< string, TList* >::iterator tree_list_itr = tree_lists.begin();
	  tree_list_itr != tree_lists.end(); tree_list_itr++) {
	
	TTree *sum_tree = TTree::MergeTrees( tree_list_itr->second );
	_DBG_ << "saving " << sum_tree->GetName() << endl;
	sum_tree->SetDirectory(the_file);
	sum_tree->Write();

	/**
	// split hnamepath into histo name and path
	//string &tnamepath = tree_list_itr->first;
	string tnamepath = tree_list_itr->first;
	size_t pos = tnamepath.find_last_of("/");
	// the path should either be be "/"
	// or the pathname not starting with '/'
	string path="";
	if(pos != string::npos) {
	    //string hname = hnamepath.substr(pos+1, hnamepath.length()-1);
	    if(tnamepath[0]=='/')
		path = tnamepath.substr(1, pos);
	    else
		path = tnamepath.substr(0, pos);
	    if(path[path.length()-1]==':')path += "/";
	} else {
	    path = "/";
	}

	_DBG_ << "saving " << sum_tree->GetName() << " in " << path << endl;

	// get the directory in which to store the histogram
	// make it if it doesn't exist 
	// (needs better error checking)
	string &path = tree_list_itr->second.path;
	TDirectory *the_dir = (TDirectory*)the_file->GetDirectory(path.c_str());
	if(!the_dir) {
	    the_file->mkdir(path.c_str());
	    the_dir = (TDirectory*)the_file->GetDirectory(path.c_str());
	}
	**/	
    }

}


