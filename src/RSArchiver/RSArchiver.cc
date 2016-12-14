// RSArchiver
// Program to archive data over RootSpy into ROOT files
// The current design of this program is to be 

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

#include "RSArchiver.h"
#include "HTMLOutputGenerator.h"
#include "PDFOutputGenerator.h"

#include "INIReader.h"

#include "rs_cmsg.h"
#include "rs_info.h"

//////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;

// classes to generate optional output formats
HTMLOutputGenerator *html_generator = NULL;
PDFOutputGenerator *pdf_generator = NULL;

static int VERBOSE = 1;  // Verbose output to screen - default is to print out some information

// configuration variables
namespace config {
    static string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
	//static string DAQ_UDL = "cMsg://127.0.0.1/cMsg";
	static string CMSG_NAME = "<not set here. see below>";
	static string SESSION = "";
	
	static TFile *CURRENT_OUTFILE = NULL;
    // name of file to keep the current state of monitoring histograms in
    // by default we also store the final state of the histograms here
	static string OUTPUT_FILENAME = "current_run.root";   
    // name of file to optionally store final histogram state in
	static string ARCHIVE_FILENAME = "<nofile>";          
	
	static double POLL_DELAY = 60;   // time between polling runs
	static double MIN_POLL_DELAY = 10;
	
	// run management 
	//bool KEEP_RUNNING = true;
	static bool FORCE_START = false;
	
	bool DONE = false;
	bool RUN_IN_PROGRESS = false;
	int RUN_NUMBER = 99999;   
	bool FINALIZE = false;
	
	static bool HTML_OUTPUT = false;
	static string HTML_BASE_DIR = "<nopath>";
	static bool PDF_OUTPUT = false;  
}

using namespace config;


//////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////

void MainLoop(void);
void GetAllHists(uint32_t Twait=2); // Twait is in seconds
void BeginRun();
void EndRunProcessing(void);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);
void WriteArchiveFile( TDirectory *sum_dir );
//void SaveTrees( TDirectoryFile *the_file );

void signal_stop_handler(int signum);

void GenerateHtmlOutput(TDirectory *the_dir, string output_basedir, string output_subdir);
void SaveFinalHistogram(map<string,server_info_t> *the_servers, final_hist_t &final_hist_info, TFile *archive_file);
void CollectFinalHistograms(map<string,server_info_t> *the_servers, string archive_filename);
string CollectFinalHistogramLoop(map<string,server_info_t> *the_servers, TFile *archive_file);

//-------------------
// IsGoodTFile
//-------------------
bool IsGoodTFile(TFile *the_file)
{
    return ((the_file!=NULL) && !(the_file->IsZombie()));
}

//-----------
// signal_stop_handler
//-----------
void signal_stop_handler(int signum)
{
    cerr << "received signal " << signum << "..." << endl;

    // let main loop know that it's time to stop
    DONE = true;
}


//-------------------
// main
//-------------------
int main(int narg, char *argv[])
{
    // Parse the command line arguments
    ParseCommandLineArguments(narg, argv);

    // register signal handlers to properly stop running
    if(signal(SIGHUP, signal_stop_handler)==SIG_ERR)
        cerr << "unable to set HUP signal handler" << endl;
    if(signal(SIGINT, signal_stop_handler)==SIG_ERR)
        cerr << "unable to set INT signal handler" << endl;
    
    
    // ------------------- initialization ---------------------------------
    BeginRun();
    
    // ------------------- periodic processing ----------------------------
    MainLoop();  // regularly poll servers for new histograms

    // ------------------- final output & cleanup -------------------------
    EndRunProcessing();   // request and collect all current histograms

    delete RS_CMSG;
    
    return 0;
}


//-----------
// BeginRun
//-----------
void BeginRun()
{
    // Create rs_info object
    RS_INFO = new rs_info();

    // ------------------- file initialization ----------------------------
    // If an output file is specified, then save histograms there instead of in memory
    if(OUTPUT_FILENAME != "<nofile>") {
        // make file to store "current" status of summed histograms
        CURRENT_OUTFILE = new TFile(OUTPUT_FILENAME.c_str(), "recreate"); 
        if(!IsGoodTFile(CURRENT_OUTFILE)) {
            cout << "Couldn't make output file, exiting" << endl;
            return;
        }
        
        // make disk-resident TDirectory
        TDirectory *dir = CURRENT_OUTFILE->mkdir("rootspy", "RootSpy local");

        // if there was a problem making the directory, no point in continuing...
        if(!dir) {
            cout << "Couldn't make rootspy directory in output file, exiting" << endl;
            return;
         }

        if(dir) {  
            // if there wasn't a problem making the directory, switch out the
            // usual TDirectory for the summed histograms in memory 
            // for the TDirectory in the file
            RS_INFO->sum_dir = dir;
        }
    }

    // optionally initialize other output formats
    if(HTML_OUTPUT)
        html_generator = new HTMLOutputGenerator("png");
    if(PDF_OUTPUT)
        pdf_generator = new PDFOutputGenerator();
    
    // Makes a Mutex to lock / unlock Root Global
    ROOT_MUTEX = new pthread_rwlock_t;
    pthread_rwlock_init(ROOT_MUTEX, NULL);
    
    // ------------------- cMsg initialization ----------------------------
    // Create cMsg object
    char hostname[256];
    gethostname(hostname, 256);
    char str[512];
    sprintf(str, "RSArchiver_%d", getpid());
    CMSG_NAME = string(str);
    cout << "Full UDL is " << ROOTSPY_UDL << endl;
    RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);

    // set session name to some default
    if(SESSION.empty()) SESSION="halldsession";
}

//-----------
// EndRunProcessing
//-----------
void EndRunProcessing(void)
{
	//if(ARCHIVE_FILENAME == "<nofile>") {
	//	cout << "No archive file specified, skipping..." << endl;
	//	return;
	//}

	if(VERBOSE>0) 
		_DBG_ << "in finalize logic..." << endl;

	RS_INFO->Lock();
	
	// ask each server for their "final" histograms
	for(map<string,server_info_t>::const_iterator server_it = RS_INFO->servers.begin();
	    server_it != RS_INFO->servers.end(); server_it++) {
		// don't ask for zero histograms, it will crash cMsg!
		if( server_it->second.hnamepaths.size() == 0 ) continue;
		
		if(VERBOSE>0) 
			_DBG_ << "sending request to" << server_it->first 
			      << " for " << server_it->second.hnamepaths.size() << " histograms " << endl;
		
		RS_CMSG->FinalHistogram(server_it->first, server_it->second.hnamepaths);
	}		

    // save the current state of which histograms are on which server so that we know what we requested
	map<string,server_info_t> *current_servers = new map<string,server_info_t>(RS_INFO->servers);
	
	RS_INFO->Unlock();
		
    // Collect the "final" histograms that we just requested
    // For now, we pass the empty string as the second argument, which tells the routine
    // to not create a new file to store the histograms in, and just update them in memory
	CollectFinalHistograms( current_servers, "" );
	
    // clean up and write out the current state of the summed histograms in memory
    // to a file before quitting
    cout << "Write all the histograms out..." << endl; 

    if(CURRENT_OUTFILE) {
        pthread_rwlock_wrlock(ROOT_MUTEX);

        RS_INFO->Lock();
        // we've mapped the directory which stores the summed histograms to a file on disk already
        // so we just need to force it to write out its current state
        RS_INFO->sum_dir->Write("",TObject::kOverwrite);
        RS_INFO->Unlock();
        
        CURRENT_OUTFILE->Close();
        pthread_rwlock_unlock(ROOT_MUTEX);
    }

}


//-----------
// MainLoop
//-----------
void MainLoop(void)
{
	if(VERBOSE>0) _DBG_ << "Running main event loop..." << endl;

    // Loop until we are told to stop for some reason	
    while(!DONE) {		    
        // keeps the connections alive, and keeps the list of servers up-to-date
        RS_CMSG->PingServers();
	
        if(VERBOSE>1)  _DBG_ << "number of servers = " << RS_INFO->servers.size() << endl;

        // Request current set of histograms 
        GetAllHists();

        // sleep for awhile. We need to break this up into a bunch of
        // smaller sleeps so when a signal is caught and we're told to
        // quit we can do so quickly before a stronger kill comes
        // along and kills us with prejudice. Sleep in 0.1sec intervals.
        //sleep(POLL_DELAY);
        int Nsleeps = (POLL_DELAY*1000000) / 100000;
        if(Nsleeps<0 || Nsleeps>10000) Nsleeps = 5; // bombproof 
        for(int i=0; i<Nsleeps; i++){
            if(DONE) {
                cout << "cleaning up and exiting..." << endl;
                break;
            }
            usleep(100000);
        }
    }
}


//-----------
// GetAllHists
//-----------
void GetAllHists(uint32_t Twait)
{
	/// This will send a request out to all servers for their list of defined
	/// histograms. It will wait for 2 seconds for them to respond and then
	/// it will send another request to every server for every histogram giving
	/// them another "Twait" seconds to respond. If Twait is not "zero" then
	/// the any output documents (html or PDF) that have been flagged for generation
	/// will be generated.
	///
	/// n.b. This routine will take a minimum of Twait+2 seconds to complete!

	// update list of histograms from all servers and give them 2 seconds to respond
	RS_CMSG->RequestHists("rootspy");
	sleep(2);

	// Request all histograms from all servers
	RS_INFO->Lock();
	map<string,hdef_t>::iterator iter = RS_INFO->histdefs.begin();
	for(; iter!=RS_INFO->histdefs.end(); iter++){
        RS_INFO->RequestHistograms(iter->first, false);
	}	
	RS_INFO->Unlock();
	
	// If Twait is "0", then return now.
	if(Twait == 0) return;
	
	// Give servers Twait seconds to respond with histograms
	if(VERBOSE>1) cout << "Waiting "<<Twait<<" seconds for servers to send histograms" << endl;
	sleep(Twait);
    
	// Lock mutexes
	RS_INFO->Lock();
	pthread_rwlock_wrlock(ROOT_MUTEX);
    
	// Save current state of summed histograms to output file
	RS_INFO->sum_dir->Write("",TObject::kOverwrite);
    
	// Optionally generate documents
	if(HTML_OUTPUT)
        html_generator->GenerateOutput(RS_INFO->sum_dir, HTML_BASE_DIR, "html");
	if(PDF_OUTPUT)
        pdf_generator->GenerateOutput(RS_INFO->sum_dir, HTML_BASE_DIR + "/html");
    
	// Unlock mutexes
	pthread_rwlock_unlock(ROOT_MUTEX);
	RS_INFO->Unlock();
}


/** -- working on this --
// assumes calling function locks the mutex
//void SaveTrees( TFile *the_file )
void SaveTrees( TDirectoryFile *the_file )
{
    _DBG_ << "In SaveTrees()..." << endl;

    // we have to keep TLists of the various trees so that we can merge them in the end.
    // the key is the full path of the tree
    map< string, TList > tree_lists;

    // for each server, loop through their trees and collect them
    for( map<string,server_info_t>::iterator server_itr = RS_INFO->servers.begin();
	 server_itr!=RS_INFO->servers.end(); server_itr++) { 

	_DBG_ << "checking " << server_itr->first << "..." << endl;

	for( vector<tree_info_t>::iterator tree_itr = server_itr->second.trees.begin();
	     tree_itr != server_itr->second.trees.end(); tree_itr++ ) {
	    _DBG_ << tree_itr->tnamepath << endl;

	    if(tree_itr->tree)
		tree_lists[ tree_itr->tnamepath ].Add( tree_itr->tree );
	}
    }

    _DBG_ << "merging trees" << endl;

    // now merge all the trees into new trees and put them in the proper place in the file
    for(  map< string, TList >::iterator tree_list_itr = tree_lists.begin();
	  tree_list_itr != tree_lists.end(); tree_list_itr++) {
	
	TTree *sum_tree = TTree::MergeTrees( &(tree_list_itr->second) );

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
	//string &path = tree_list_itr->second.path;
	TDirectory *the_dir = (TDirectory*)the_file->GetDirectory(path.c_str());
	if(!the_dir) {
	    the_file->mkdir(path.c_str());
	    the_dir = (TDirectory*)the_file->GetDirectory(path.c_str());
	}	
    }

}
**/


//-----------
// SaveFinalHistogram
//-----------
void SaveFinalHistogram(final_hist_t &final_hist_info, TFile *archive_file)
{
    string &hnamepath = final_hist_info.hnamepath;
    TH1 *h = final_hist_info.hist;
            
    if(VERBOSE>0) 
        _DBG_<<"it contained histogram "<<hnamepath<<endl;
    if(VERBOSE>2)
        h->Print();
    	    
    // split hnamepath into histo name and path
    size_t pos = hnamepath.find_last_of("/");
    
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
                
    if(VERBOSE>0) {
        _DBG_<<"saving histogram (" << hnamepath << ")..."<<endl;
        _DBG_<<"histogram path = \"" << path << "\"" << endl;
    }

    // let's see if the histogram exists in our output file yet
    TH1 *hfinal = (TH1*)archive_file->Get(hnamepath.c_str());
        
    if(!hfinal) {
        // If the histogram doesn't exist in the output file, then let's make it!

        // get the directory in which to store the histogram
        // and make it if it doesn't exist 
        // (needs better error checking)
        TDirectory *hist_dir = (TDirectory*)archive_file->GetDirectory(path.c_str());
        if(!hist_dir) {
            archive_file->mkdir(path.c_str());
            hist_dir = (TDirectory*)archive_file->GetDirectory(path.c_str());
        }

        if(VERBOSE>0) {
            _DBG_<<"histogram path = \"" << path << "\"  " << hist_dir << endl;
            if(VERBOSE>2)
                hist_dir->Print();
        }
        
        // put the histogram in the right location
        // once it's file bound, we dont' have to worry about
        // freeing the memory (I think)
        h->SetDirectory(hist_dir);
        //h->Write();
    } else {
        // if we do find the histogram in the output file, add our result to it
        hfinal->Add(h);
        //hfinal->Write();   // do we need to do this every time?
        delete h;  // we can finally release this memory
    }
    
    pthread_rwlock_unlock(ROOT_MUTEX);

}

//-----------
// CollectFinalHistogramLoop
//-----------
string CollectFinalHistogramLoop(map<string,server_info_t> *the_servers, TFile *archive_file)
{
    const int FINAL_TIMEOUT = 5;       // only wait 5 minutes for everyone to report in
    const int FINAL_LOOP_DELAY = 10000;  // period of time to wait in between checking for more histograms that have been sent to us (in us)

    // save the time so that we can decide when to stop processing
    time_t now = time(NULL);
    time_t start_time = now;
    
    // keep track of which servers we've received responses from
    stringstream responded_server_stream;
    
    while(true) {
        // eventually give up waiting...
        if( now - start_time > FINAL_TIMEOUT ) {
            _DBG_<<"final histogram collection timing out..."<<endl;
            break;
        }

        RS_INFO->Lock();

        if(VERBOSE>0) { 
            _DBG_<<" number of servers left = "<<the_servers->size()
                 <<" number of queued messages = "<<RS_INFO->final_hists.size()<<endl;
            cout << "we are still waiting on these servers: ";
            
            for( map<string,server_info_t>::iterator server_itr = the_servers->begin();
                 server_itr!=the_servers->end(); server_itr++) { 
                cout << server_itr->first << " ";
            }
            cout << endl;
        }
        
        // process all incoming histograms
        while( !RS_INFO->final_hists.empty() ) {
            // extract histogram information
            final_hist_t &final_hist_info = RS_INFO->final_hists.front();
            string &sender = final_hist_info.serverName;
            string &hnamepath = final_hist_info.hnamepath;
                
            if(VERBOSE>0) 
                _DBG_<<"got message from " << sender << endl;
                
            // make sure that the sender is one of the servers we are waiting
            map<string,server_info_t>::iterator server_info_iter = the_servers->find(sender);
            if(server_info_iter!=the_servers->end()) {

                if(archive_file != NULL) {
                    SaveFinalHistogram(final_hist_info, archive_file);
                }

                // remove histogram from list of hists we are waiting for
                if(VERBOSE>2) 
                    _DBG_ << "num hists before erase " << server_info_iter->second.hnamepaths.size() << endl;

                vector<string>::iterator hnamepath_itr = find(server_info_iter->second.hnamepaths.begin(),
                                                              server_info_iter->second.hnamepaths.end(),
                                                              hnamepath);
                if(hnamepath_itr!=server_info_iter->second.hnamepaths.end())
                    server_info_iter->second.hnamepaths.erase(hnamepath_itr);
                else
                    _DBG_ << "couldn't find histogram!" << endl;
                
                if(VERBOSE>2) 
                    _DBG_ << "num hists after erase " << server_info_iter->second.hnamepaths.size() << endl;
            } else {
                // error reporting is good
                _DBG_ << " received message from invalid server in ArchiveFile()" << endl;
            }
            
            // if we aren't waiting for any more histograms from this server, then we can remove it from the list
            if(server_info_iter->second.hnamepaths.empty()) {
                responded_server_stream << sender << " ";
                the_servers->erase(sender);
            }

            // we've finished with the message, so we can get rid of it
            RS_INFO->final_hists.pop_front();
        }
        
        RS_INFO->Unlock();
        
        // stop when we get all the responses
        if( the_servers->empty() )
            break;	    
	
        // wait for more histograms
        usleep(FINAL_LOOP_DELAY);
    }

    return responded_server_stream.str();
}


//-----------
// CollectFinalHistograms
//-----------
void CollectFinalHistograms(map<string,server_info_t> *the_servers, string archive_filename="")
{
    // map of the set of servers and histograms that we want to save
    // should have already been allocated before starting the thread
    //map<string,server_info_t> *the_servers = static_cast< map<string,server_info_t> *>(ptr);
    const bool save_in_new_file = (archive_filename!="");

    if(VERBOSE>0) 
        _DBG_<<"Starting in ArchiveFile()..."<<endl;
    
    // preprocessing: clean up server list - any servers containing zero histograms should be removed
    for( map<string,server_info_t>::iterator server_itr = the_servers->begin();
         server_itr!=the_servers->end(); server_itr++) { 
        if(server_itr->second.hnamepaths.empty()) {
            the_servers->erase(server_itr);
        }
    }

    // optionally create a new file to save the results in
    TFile *archive_file = NULL;
    if(save_in_new_file) {
        pthread_rwlock_wrlock(ROOT_MUTEX);
        archive_file = new TFile(archive_filename.c_str(), "create");
        if(!IsGoodTFile(archive_file)) {
            if(VERBOSE>1) _DBG_<<"  Unable to create good archive file \""<<ARCHIVE_FILENAME<<"\". Bailing."<<endl;
            pthread_rwlock_unlock(ROOT_MUTEX);
            delete the_servers;
            return;
        }
        pthread_rwlock_unlock(ROOT_MUTEX);
    }


    // start looping to collect all the histograms
    string responded_servers = CollectFinalHistogramLoop(the_servers, archive_file);
    
    if(VERBOSE>0) 
        _DBG_<<"Finishing up ArchiveFile()..."<<endl;

    // postprocessing
    if(save_in_new_file) {
        // save any extra data that you want
        archive_file->mkdir("RootSpyInfo");
        archive_file->cd("RootSpyInfo");
        TNamed responded_servers_obj("servers", responded_servers.c_str());
        responded_servers_obj.Write();

        // close file and clean up
        pthread_rwlock_wrlock(ROOT_MUTEX);
        archive_file->Write();
        archive_file->Close();
        
        delete archive_file;
        delete the_servers;
        pthread_rwlock_unlock(ROOT_MUTEX);
    }
}


//-----------
// ParseCommandLineArguments
// configuration priorities (low to high):
//    config file -> environmental variables -> command line
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
    if(VERBOSE>0) 
        _DBG_ << "In ParseCommandLineArguments().." << endl;
    
    
    // read from configuration file
    // TODO: decide how to set config filename on command line
    // the complication is that the config file settings should have the lowest priority
    string config_filename = getenv("HOME");  // + "/.RSArchiver";
    config_filename += "/.RSArchiver";
    const char *configptr = getenv("ROOTSPY_CONFIG_FILENAME");
    if(configptr) config_filename = configptr;  
    
    INIReader config_reader(config_filename.c_str());
    
    if (config_reader.ParseError() < 0) {
        std::cout << "Can't load configuration file '" << config_filename << "'" << endl;
    } else {
        
        // [main]
        ROOTSPY_UDL = config_reader.Get("main", "rootspy_udl",  "cMsg://127.0.0.1/cMsg/rootspy");
        //DAQ_UDL     = config_reader.Get("main", "daq_udl",      "cMsg://127.0.0.1/cMsg");
        //CMSG_NAME   = config_reader.Get("main", "cmsg_name",    "");   // dynamically generated
        SESSION     = config_reader.Get("main", "session_name", "");
        
        OUTPUT_FILENAME  = config_reader.Get("main", "output_filename", "current_run.root");
        ARCHIVE_FILENAME = config_reader.Get("main", "archive_filename", "<nofile>");
        
        POLL_DELAY      = config_reader.GetInteger("main", "poll_delay", 60);
        MIN_POLL_DELAY  = config_reader.GetInteger("main", "min_poll_delay", 10);
        
        // [output]
        HTML_OUTPUT   = config_reader.GetBoolean("output", "html_output",   false);
        PDF_OUTPUT    = config_reader.GetBoolean("output", "pdf_output",    false);
        HTML_BASE_DIR = config_reader.Get("output", "html_base_dir", "<nopath>");

    }
    
    // allow for environmental variables
    const char *ptr = getenv("ROOTSPY_UDL");
    if(ptr)ROOTSPY_UDL = ptr;
    
    const char *ptr2 = getenv("ROOTSPY_ARCHIVE_FILENAME");
    if(ptr2)ARCHIVE_FILENAME = ptr2;
    
    // check command line options
    static struct option long_options[] = {
        {"help",           no_argument,       0,  'h' },
        {"run-number",     required_argument, 0,  'R' },
        {"poll-delay",     required_argument, 0,  'p' },
        //{"min-poll-delay", required_argument, 0,  'm' },
        {"udl",            required_argument, 0,  'u' },
        //{"daq-udl",        required_argument, 0,  'q' },
        {"server",         required_argument, 0,  's' },
        {"archive-file",   required_argument, 0,  'A' },
        {"output-file",    required_argument, 0,  'F' },
	
        {"pdf-output",     no_argument,       0,  'P' },
        {"html-output",    no_argument,       0,  'H' },
        {"session-name",   no_argument,       0,  'S' },
        {"summary-dir",    required_argument, 0,  'Y' },
        {"verbose",        no_argument,       0,  'V' },

        {0, 0, 0, 0  }
    };
    
    int opt = 0;
    int long_index = 0;
    while ((opt = getopt_long(narg, argv,"hR:p:u:s:A:F:PHY:v", 
                              long_options, &long_index )) != -1) {
        switch (opt) {
        case 'R':
            if(optarg == NULL) Usage();
            RUN_NUMBER = atoi(optarg);
        case 'f' : 
            FORCE_START = true;
            break;
        case 'p' : 
            if(optarg == NULL) Usage();
            POLL_DELAY = atoi(optarg);
            break;
        case 'u' :
            if(optarg == NULL) Usage();
            ROOTSPY_UDL = optarg;
            break;
            //case 'q' :
            //if(optarg == NULL) Usage();
            //DAQ_UDL = optarg;
            //break;
        case 's' :
            if(optarg == NULL) Usage();
            ROOTSPY_UDL = "cMsg://";
            ROOTSPY_UDL += optarg;
            ROOTSPY_UDL += "/cMsg/rootspy";
            break;
        case 'A' :
            if(optarg == NULL) Usage();
            ARCHIVE_FILENAME = optarg;
            break;
        case 'F' :
            if(optarg == NULL) Usage();
            OUTPUT_FILENAME = optarg;
            break;
        case 'Y' :
            if(optarg == NULL) Usage();
            HTML_BASE_DIR = optarg;
            break;
            //case 'n' :
            //if(optarg == NULL) Usage();
            //CMSG_NAME = optarg;
            //break;
        case 'P' :
            PDF_OUTPUT = true;
            break;
        case 'H' :
            HTML_OUTPUT = true;
            break;
        case 'S' :
            SESSION = optarg;
            break;
        case 'v' :
            VERBOSE++;
            break;
            
        case 'h':
        default: Usage(); break;
        }
    }  
  

    // check any values
    if(POLL_DELAY < MIN_POLL_DELAY)
        POLL_DELAY = MIN_POLL_DELAY;

    // DUMP configuration
    cout << "-------------------------------------------------" << endl;
    cout << "Current configuration:" << endl;
    cout << "ROOTSPY_UDL = " << ROOTSPY_UDL << endl;
    //cout << "DAQ_UDL = " << DAQ_UDL << endl;
    cout << "SESSION = " << SESSION << endl;

    cout << "RUN_NUMBER = " << RUN_NUMBER << endl;
    cout << "OUTPUT_FILENAME = " << OUTPUT_FILENAME << endl;
    cout << "ARCHIVE_FILENAME = " << ARCHIVE_FILENAME << endl;
    cout << "POLL_DELAY = " << POLL_DELAY << endl;
    cout << "MIN_POLL_DELAY = " << MIN_POLL_DELAY << endl;
    
    cout << "HTML_OUTPUT = " << HTML_OUTPUT << endl;
    cout << "PDF_OUTPUT = " << PDF_OUTPUT << endl;
    cout << "HTML_BASE_DIR = " << HTML_BASE_DIR << endl;

    cout << "-------------------------------------------------" << endl;

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
    cout<<"   -h,--help                 Print this message"<<endl;
    cout<<"   -u,--udl udl              UDL of cMsg RootSpy server"<<endl;
    cout<<"   -s,--server server-name   Set RootSpy UDL to point to server IP/hostname"<<endl;
    cout<<"   -R,--run-number number    The number of the current run" << endl;
    cout<<"   -p,--poll-delay time      Time (in seconds) to wait between polling seconds" << endl;
    //cout<<"   -m,--min-poll-delay time  Time"<<endl;

    cout<<"   -F,--output-file file     Name of ROOT file used during run" << endl;
    cout<<"   -A,--archive-file file    Name of ROOT file used to archive the final results" << endl;
    cout<<"   -P,--pdf-output           Enable output of summary PDF"<<endl;
    cout<<"   -H,--html-output          Enable output of summary web pages"<<endl;
    cout<<"   -Y,--summary-dir path     Directory used to store summary files"<<endl;
    cout<<"   -v,--verbose              Increase verbosity (can use multiple times)"<<endl;
    cout<<endl;
    cout<<endl;
    
    exit(0);
}

