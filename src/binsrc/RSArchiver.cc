
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <TFile.h>
#include <TDirectory.h>
#include <TObjArray.h>
#include <TH1F.h>
#include <TClass.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "RSArchiver.h"
#include "rs_cmsg.h"
#include "rs_info.h"
#include "rs_mainframe.h"


// GLOBALS
// should set a namespace for these?
rs_mainframe *RSMF = NULL;
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;

// configuration variables
string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
string CMSG_NAME = "<not set here. see below>";
string OUTPUT_FILENAME = "current_run.root";

TFile *CURRENT_OUTFILE = NULL;
bool KEEP_RUNNING = true;
double POLL_DELAY = 30;   // time between polling runs, default 1 min?
//const double MIN_POLL_DELAY = 10;
const double MIN_POLL_DELAY = 5;

 int RUN_NUMBER = 1;
//string ARCHIVE_PATHNAME = "/u/home/sdobbs/test_archives";  // DEFAULT FOR TESTING
string ARCHIVE_PATHNAME = "<nopath>";


// communication variables
bool SWITCH_FILES = false;
bool FINALIZE = false;

vector<pthread_t> thread_ids;


void MainLoop();
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);
void SaveDirectory( TDirectory *the_dir, TFile *the_file );
void WriteArchiveFile( TDirectory *sum_dir );

void signal_stop_handler(int signum);
void signal_switchfile_handler(int signum);
void signal_finalize_handler(int signum);

void *ArchiveFile(void * ptr);

bool IsGoodTFile(TFile *the_file)
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

    if(ARCHIVE_PATHNAME == "<nopath>") {
	char *cwd_buf = getcwd(NULL, 0);
	//char *cwd_buf = get_current_dir_name();
	ARCHIVE_PATHNAME = cwd_buf;
	free(cwd_buf);
    }

    // register signal handlers to properly stop running
    signal(SIGHUP, signal_stop_handler);
    signal(SIGINT, signal_stop_handler);

    // and some for IPC (for now..)
    signal(SIGUSR1, signal_switchfile_handler);
    signal(SIGUSR2, signal_finalize_handler);


    // make file to store "current" status of summed histograms
    CURRENT_OUTFILE = new TFile(OUTPUT_FILENAME.c_str(), "recreate"); 
    if(!IsGoodTFile(CURRENT_OUTFILE)) {
	cout << "Couldn't make outpit file, exiting" << endl;
	return 0;
    }

    // make disk-resident TDirectory
    TDirectory *dir = CURRENT_OUTFILE->mkdir("rootspy", "RootSpy local");

    // if there was a problem making the directory, no point in continuing...
    if(!dir) {
	cout << "Couldn't make rootspy directory, exiting" << endl;
	return 0;
    }

    // Create rs_info object
    RS_INFO = new rs_info();
    if(dir) {  
	// if there wasn't a problem making the directory, switch out the
	// usual TDirectory for the summed histograms in memory 
	// for the TDirectory in the file
	RS_INFO->sum_dir = dir;
    };

    
    // Makes a Mutex to lock / unlock Root Global
    ROOT_MUTEX = new pthread_rwlock_t;
    pthread_rwlock_init(ROOT_MUTEX, NULL);
    
    // Create cMsg object
    char hostname[256];
    gethostname(hostname, 256);
    char str[512];
    sprintf(str, "RootSpy Archiver %s-%d", hostname, getpid());
    CMSG_NAME = string(str);
    cout << "Full UDL is " << ROOTSPY_UDL << endl;
    RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);


    //  regularly poll servers for new histograms
    MainLoop();


    // clean up and write out the current state of the summed histograms to a file
    // before quitting
    cout << "Write all the histograms out..." << endl; 

    // dump summed histograms to file
    pthread_rwlock_wrlock(ROOT_MUTEX);

    RS_INFO->Lock();
    RS_INFO->sum_dir->Write("",TObject::kOverwrite);
    RS_INFO->Unlock();

    CURRENT_OUTFILE->Close();
    pthread_rwlock_unlock(ROOT_MUTEX);


    delete RS_CMSG;
    delete RSMF;
    
    return 0;
}


//-----------
// MainLoop
//-----------
void MainLoop()
{
    while(KEEP_RUNNING) {
	
	cout << "Running main event loop..." << endl;

	_DBG_ << "number of servers = " << RS_INFO->servers.size() << endl;
	
	// keeps the connections alive, and keeps the list of servers up-to-date
	RS_CMSG->PingServers();


	RS_INFO->Lock();
	// update list of histograms
	for(map<string,server_info_t>::const_iterator server_it = RS_INFO->servers.begin();
	    server_it != RS_INFO->servers.end(); server_it++) {
	    RS_CMSG->RequestHists(server_it->first);
	    RS_CMSG->RequestTreeInfo(server_it->first);
	}


	// get/save information
	for(map<string,server_info_t>::const_iterator server_it = RS_INFO->servers.begin();
	    server_it != RS_INFO->servers.end(); server_it++) {

	    // get/save current histogram versions
	    for(vector<string>::const_iterator hist_it = server_it->second.hnamepaths.begin();
		hist_it != server_it->second.hnamepaths.end(); hist_it++) {
		RS_CMSG->RequestHistogram(server_it->first, *hist_it);
	    }

	    // get/save current status of trees
	    for(vector<tree_info_t>::const_iterator tree_it = server_it->second.trees.begin();
		tree_it != server_it->second.trees.end(); tree_it++) {
		RS_CMSG->RequestTree(server_it->first, tree_it->name, tree_it->path);
	    }
	}
	


	// save current state of summed histograms
	pthread_rwlock_wrlock(ROOT_MUTEX);
	RS_INFO->sum_dir->ls();
	RS_INFO->sum_dir->Write("",TObject::kOverwrite);
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
    

	// make sure we sleep for the full amount even if we get hit with a signal
	// so that we can use signals to test begin/end run behavior
	// NOTE that we should really do this with cMsg, or something like that, 
	// whenever some canonical behavior is defined
	time_t now = time(NULL);
	time_t start_time = now;

	while(now < start_time+POLL_DELAY) {  

	    // handle signals

	    // time to end the run, save the current file, and start a new one
	    // This should also be moved to a cMsg handler at some point
	    // more for testing, probably don't need this anymore
	    /*
	    if(SWITCH_FILES) {
		RS_INFO->Lock();
		WriteArchiveFile( RS_INFO->sum_dir );
		RS_INFO->Unlock();
		
		RUN_NUMBER += 1;    // mutex lock?
		SWITCH_FILES=false;
	    }
	    */
	    
	    // End-of-run logic
	    if(FINALIZE) {
		RS_INFO->Lock();
		pthread_t the_thread; 
		
		// ask each server for their "final" histograms
		for(map<string,server_info_t>::const_iterator server_it = RS_INFO->servers.begin();
		    server_it != RS_INFO->servers.end(); server_it++) {
		    // don't ask for zero histograms, it will crash cMsg
		    if( server_it->second.hnamepaths.size() == 0 ) continue;

		    //_DBG_ << "sending request to" << server_it->first.serverName 
		    _DBG_ << "sending request to" << server_it->first 
			  << " for " << server_it->second.hnamepaths.size() << " histograms " << endl;

		    RS_CMSG->FinalHistogram(server_it->first, server_it->second.hnamepaths);
		}		
		// make a thread to handle collecting the final histograms
		pthread_create(&the_thread, NULL, ArchiveFile, 
			       new map<string,server_info_t>(RS_INFO->servers) ); 
		thread_ids.push_back(the_thread);
		RS_INFO->Unlock();
		
		//RUN_NUMBER += 1;    // mutex lock?
		FINALIZE=false;
	    }


	    sleep(POLL_DELAY - (now-start_time));
	    now = time(NULL);

	    if(!KEEP_RUNNING) {
		// for testing...
		for(vector<pthread_t>::iterator tid_it=thread_ids.begin();
		    tid_it!=thread_ids.end(); tid_it++) {
		    pthread_cancel(*tid_it);
		}
		
		break;
	    }
	}
    }
}


void signal_stop_handler(int signum)
{
    cerr << "received signal " << signum << "..." << endl;
    cerr << "cleaning up and exiting..." << endl;

    // let main loop know that it's time to stop
    KEEP_RUNNING = false;
}

void signal_switchfile_handler(int signum)
{
    SWITCH_FILES = true;
}

void signal_finalize_handler(int signum)
{
    FINALIZE = true;
}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
    // Copy from environment first so that command line may
    // override
    const char *ptr = getenv("ROOTSPY_UDL");
    if(ptr)ROOTSPY_UDL = ptr;

    const char *ptr2 = getenv("ROOTSPY_ARCHIVE_PATHNAME");
    if(ptr2)ARCHIVE_PATHNAME = ptr2;
  
    // Loop over command line arguments
    for(int i=1;i<narg;i++){
	if(argv[i][0] != '-')continue;
	switch(argv[i][1]){
	case 'h':
	    Usage();
	    break;
	case 'u':
	    if(i>=(narg-1)){
		cerr<<"-u option requires an argument"<<endl;
	    }else{
		ROOTSPY_UDL = argv[i+1];
	    }
	    break;
	case 'n':
	    if(i>=(narg-1)){
		cerr<<"-n option requires an argument"<<endl;
	    }else{
		CMSG_NAME = argv[i+1];
	    }
	    break;
	case 'f':
	    if(i>=(narg-1)){
		cerr<<"-f option requires an argument"<<endl;
	    }else{
		OUTPUT_FILENAME = argv[i+1];
	    }
	    break;
	case 'A':
	    if(i>=(narg-1)){
		cerr<<"-A option requires an argument"<<endl;
	    }else{
		ARCHIVE_PATHNAME = argv[i+1];
	    }
	    break;
	case 'p':
	    if(i>=(narg-1)){
		cerr<<"-p option requires an argument"<<endl;
	    }else{
		POLL_DELAY = atoi(argv[i+1]);
		if(POLL_DELAY < MIN_POLL_DELAY)
		    POLL_DELAY = MIN_POLL_DELAY;
	    }
	    break;
	case 's':
	    if(i>=(narg-1)){
		cerr<<"-s option requires an argument"<<endl;
	    }else{
		ROOTSPY_UDL = "cMsg://";
		ROOTSPY_UDL += argv[i+1];
		ROOTSPY_UDL += "/cMsg/rootspy";
	    }
	    break;
	}
    }
}

//-----------
// Usage
//-----------
void Usage(void)
{
    cout<<"Usage:"<<endl;
    cout<<"       RSArchiver [options]"<<endl;
    cout<<endl;
    cout<<"Saves ROOT histograms being stored by programs running the RootSpy plugin."<<endl;
    cout<<endl;
    cout<<"Options:"<<endl;
    cout<<endl;
    cout<<"   -h        Print this message"<<endl;
    cout<<"   -u udl    UDL of cMsg server (def. "<<ROOTSPY_UDL<<")"<<endl;
    cout<<"   -n name   Specify name this program registers with cMsg server"<<endl;
    cout<<"             (def. "<<CMSG_NAME<<")"<<endl;
    cout<<"   -A path   Root directory of archives (def. ?)" <<endl;
    cout<<"   -f fn     Name of ROOT file to store cumulative output in"
	<<"(def. "<<OUTPUT_FILENAME<<")"<<endl;
    cout<<"   -p time   Time (in seconds) to wait in between polling cycles"
	<<"( def. "<<POLL_DELAY<<"?)"<<endl;
    cout<<endl;
    
    exit(0);
}

// write out a directory of ROOT objects to an archived file
// do we need this?
void WriteArchiveFile( TDirectory *sum_dir )
{
    stringstream ss;
    ss << ARCHIVE_PATHNAME << "/" << "run" << RUN_NUMBER << "_output.root";
    
    pthread_rwlock_wrlock(ROOT_MUTEX);
    //RS_INFO->Lock();	    
    
    TFile *archive_file = new TFile(ss.str().c_str(), "create");
    if(IsGoodTFile(archive_file)) {
	//archive_file->mkdir("rootspy", "");
	SaveDirectory( sum_dir, archive_file );
	
	archive_file->Close();
	delete archive_file;
    } // else {
    //cout << "could not create archive file: " << ss.str() << endl;
    //}

    //RS_INFO->Unlock();
    pthread_rwlock_unlock(ROOT_MUTEX);


}

// recursively copy/save a directory to a file
void SaveDirectory( TDirectory *the_dir, TFile *the_file )
{
    if(the_dir == NULL)
	return;
    
    string path = the_dir->GetPath();

    TList *list = the_dir->GetList();
    TIter next(list);

    while(TObject *obj = next()) {
	TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
	if(subdir!=NULL && subdir!=the_dir) {
	    string new_subdir_path = path + "/" + obj->GetName();
	    TDirectory *new_subdir = the_file->mkdir(new_subdir_path.c_str(), "");
	    new_subdir->cd();
	    SaveDirectory(subdir, the_file);
	} else {
	    TObject *new_obj = obj->Clone();
	    new_obj->Write();
	}

    }

}


// function for saving end-of-run histograms
// use server info map to keep track of which histograms we're waiting for
void *ArchiveFile(void * ptr) 
{
    // map of the set of servers and histograms that we want to save
    // should have already been allocated before starting the thread
    map<string,server_info_t> *the_servers = static_cast< map<string,server_info_t> *>(ptr);
    const int FINAL_TIMEOUT = 600;   // only wait 5 minutes for everyone to report in

    _DBG_<<"Starting in ArchiveFile()..."<<endl;

    // clean up server list - any servers containing zero histograms should be removed
    for( map<string,server_info_t>::iterator server_itr = the_servers->begin();
	 server_itr!=the_servers->end(); server_itr++) { 
	if(server_itr->second.hnamepaths.empty()) {
	    the_servers->erase(server_itr);
	}
    }


    // make file and directory to save histograms in
    stringstream ss;
    ss << ARCHIVE_PATHNAME << "/" << "run" << RUN_NUMBER << "_output.root";
    
    pthread_rwlock_wrlock(ROOT_MUTEX);
    TFile *archive_file = new TFile(ss.str().c_str(), "create");
    if(!IsGoodTFile(archive_file)) {
	delete the_servers;
	return NULL;
    }
    
    //TDirectory *out_dir = archive_file->mkdir("rootspy", ""); // don't need this?
    pthread_rwlock_unlock(ROOT_MUTEX);


    // start looping to collect all the histograms
    time_t now = time(NULL);
    time_t start_time = now;
	
    while(true) {
	// eventually give up and write out to disk
	if( now - start_time > FINAL_TIMEOUT ) {
	    _DBG_<<"final histogram collection timing out..."<<endl;
	    break;
	}


	RS_INFO->Lock();

	_DBG_<<" number of servers left = "<<the_servers->size()
	     <<" number of queued messages = "<<RS_INFO->final_hists.size()<<endl;
	cout << "we are still waiting on these servers: ";
	for( map<string,server_info_t>::iterator server_itr = the_servers->begin();
	     server_itr!=the_servers->end(); server_itr++) { 
	    cout << server_itr->first << " ";
	}
	cout << endl;

	
	//for(deque<cMsgMessage>::iterator it_cmsg = RS_INFO->final_messages.begin();
	//it_cmsg != RS_INFO->final_messages.end(); it_cmsg++) {
	  
	// process all incoming histograms
	while( !RS_INFO->final_hists.empty() ) {
	    // extract histogram information
	    final_hist_t &final_hist_info = RS_INFO->final_hists.front();
	    string &sender = final_hist_info.serverName;
	    string &hnamepath = final_hist_info.hnamepath;
	    TH1 *h = final_hist_info.hist;
	    
	    _DBG_<<"got message from " << sender << endl;
	    
	    // make sure that the sender is one of the servers we are waiting
	    map<string,server_info_t>::iterator server_info_iter = the_servers->find(sender);
	    if(server_info_iter!=the_servers->end()) {

		_DBG_<<"it contained histogram "<<hnamepath<<endl;
		h->Print();

		// Lock ROOT mutex while working with ROOT objects
		pthread_rwlock_wrlock(ROOT_MUTEX);
	    
		// split hnamepath into histo name and path
		size_t pos = hnamepath.find_last_of("/");
		//string hname="",path="";
		
		// the path should either be be "/"
		// or the pathname not starting with '/'
		string path="";
		if(pos != string::npos) {
		    //string hname = hnamepath.substr(pos+1, hnamepath.length()-1);
		    if(hnamepath[0]=='/')
			path = hnamepath.substr(1, pos);
		    else
			path = hnamepath.substr(0, pos);
		    if(path[path.length()-1]==':')path += "/";
		} else {
		    path = "/";
		}


		_DBG_<<"saving histogram (" << hnamepath << ")..."<<endl;
		_DBG_<<"histogram path = \"" << path << "\"" << endl;

		// let's see if the histogram exists alrady
		TH1 *hfinal = (TH1*)archive_file->Get(hnamepath.c_str());
	    
		if(!hfinal) {
		    // if we don't get the histogram, then it must not exist yet!
		    // let's fix that

		    // get the directory in which to store the histogram
		    // make it if it doesn't exist 
		    // (needs better error checking)
		    TDirectory *hist_dir = (TDirectory*)archive_file->GetDirectory(path.c_str());
		    if(!hist_dir) {
			//hist_dir = archive_file->mkdir(path.c_str());
			archive_file->mkdir(path.c_str());
			hist_dir = (TDirectory*)archive_file->GetDirectory(path.c_str());
		    }

		    _DBG_<<"histogram path = \"" << path << "\"  " << hist_dir << endl;
		    hist_dir->Print();
		
		    // put the histogram in the right location
		    // once it's file bound, we dont' have to worry about
		    // freeing the memory (I think)
		    h->SetDirectory(hist_dir);
		    //h->Write();
		} else {
		    // if we do find the histogram on disk, add to it
		    hfinal->Add(h);
		    //hfinal->Write();   // do we need to do this everytime?
		    delete h;  // we can finally release this memory
		}

		
		pthread_rwlock_unlock(ROOT_MUTEX);
		
		// remove histogram from list of hists we are waiting for
		_DBG_ << "num hists before erase " << server_info_iter->second.hnamepaths.size() << endl;
		vector<string>::iterator hnamepath_itr = find(server_info_iter->second.hnamepaths.begin(),
							      server_info_iter->second.hnamepaths.end(),
							      hnamepath);
		if(hnamepath_itr!=server_info_iter->second.hnamepaths.end())
		    server_info_iter->second.hnamepaths.erase(hnamepath_itr);
		else
		    _DBG_ << "couldn't find histogram!" << endl;
		
		_DBG_ << "num hists after erase " << server_info_iter->second.hnamepaths.size() << endl;
	    } else {
		// error reporting is good
		_DBG_ << " received message from invalid server in ArchiveFile()" << endl;
	    }
		
	    // if we aren't waiting for any more histograms from this server, then kill it
	    if(server_info_iter->second.hnamepaths.empty())
		the_servers->erase(sender);
	    RS_INFO->final_hists.pop_front();
	    
	}
	
	RS_INFO->Unlock();

	// stop when we get all the responses
	if( the_servers->empty() )
	    break;	    
	
	// wait for a second for more histograms
	sleep(1);
    }
    
    _DBG_<<"Finishing up ArchiveFile()..."<<endl;

    // close file and clean up
    pthread_rwlock_wrlock(ROOT_MUTEX);
    //out_dir->Write();
    archive_file->Write();
    archive_file->Close();

    delete archive_file;
    delete the_servers;
    pthread_rwlock_unlock(ROOT_MUTEX);
    
    RUN_NUMBER += 1;    // mutex lock, yes, could have multiple runs

    // thread is over
}
