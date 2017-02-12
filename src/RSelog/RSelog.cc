// RSArchiver
// Program to archive data over RootSpy into ROOT files
// The current design of this program is to be 

#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#include <TROOT.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>
#include <TClass.h>
#include <TTree.h> 
#include <TStyle.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <mutex>
using namespace std;


#include "rs_cmsg.h"
#include "rs_info.h"

//////////////////////////////////////////////
// GLOBALS
//////////////////////////////////////////////
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;

// These defined in rs_cmsg.cc
extern mutex REGISTRATION_MUTEX;
extern map<void*, string> HISTOS_TO_REGISTER;
extern map<void*, string> MACROS_TO_REGISTER;


static int VERBOSE = 1;  // Verbose output to screen - default is to print out some information

// configuration variables
static string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
static string CMSG_NAME = "<not set here. see below>";
static set<string> HNAMEPATHS;

bool DONE = false;
int RUN_NUMBER = 99999;   


//////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////

void signal_stop_handler(int signum);

bool GetHists(const set<string> &hnamepaths, uint32_t timeout_secs=5);
void RegisterQueuedItems(void);
void ExecuteMacro(TDirectory *f, string macro);
void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);


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
	if(signal(SIGHUP, signal_stop_handler)==SIG_ERR) cerr << "unable to set HUP signal handler" << endl;
	if(signal(SIGINT, signal_stop_handler)==SIG_ERR) cerr << "unable to set INT signal handler" << endl;

	// Create a lock for controlling access to ROOT Global
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);

	// Create rs_info object
	RS_INFO = new rs_info();

	// ------------------- cMsg initialization ----------------------------
	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "RSelog_%d", getpid());
	CMSG_NAME = string(str);
	cout << "RootSpy UDL is " << ROOTSPY_UDL << endl;
	RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
	
	// Wait for RS_CMSG to launch cMsg threads and ping servers
	sleep(2);
	
	// Request all histogram definitions first so RootSpy will know what to do with them
	// when the histograms come.
	cout << endl << "------------------------------------------------------" << endl;
	cout << "Requesting all histogram definitions ... " << endl;
	RS_CMSG->RequestHists("rootspy");
	usleep(500000);
	
	// Initial request of all hnamepaths. Most likely these are macros
	// se we will need to send out second requests later for the actual
	// histograms.
	bool good = GetHists(HNAMEPATHS);
	if( !good ){
		cerr << "Unable to get all hnamepaths in initial set!" << endl;
		exit(-1);
	}

	// Create TCanvas, but set ROOT to batch mode so that it doesn't actually open a window.
	gROOT->SetBatch(kTRUE);
	TCanvas *c1 = new TCanvas("c1", "A Canvas", 1600, 1200);
	
	// Loop over hnamepaths, drawing each into the canvas and saving the image
	int ipage=1;
	for(string hnamepath : HNAMEPATHS){
		RS_INFO->Lock();
		
		for(string s : HNAMEPATHS){
		
			cout << endl << "--- Processing " << s << " ---" << endl;
		
			// Get pointer to hdef for this hnamepath
			const hdef_t *hdef = NULL;
			for(auto hdefpair : RS_INFO->histdefs){
				if( s == hdefpair.second.hnamepath ) {hdef = &hdefpair.second; break;}
			}
			if(!hdef) {
				cerr << "Hmmm ... could not find hdef for " << s << " but it should be here!?!? -- skipping" << endl;
				continue;
			}
			
			// Get any additional hnamepaths this may require (if it is a macro)
			bool skip = false;
			if( ! hdef->macro_hnamepaths.empty() ){
				bool good = GetHists(hdef->macro_hnamepaths);
				if( !good ){
					cerr << "Could not find all hnamepaths needed for macro " << s << " -- skipping " << endl;
					skip = true;
				}
			}
			if(skip) continue;
			
			// Clear canvas
			c1->Clear();

			// Check if this is a macro or regular histogram
			if(hdef->type == hdef_t::macro){
				const hinfo_t &hinfo = hdef->hists.begin()->second; // Use first hinfo
				ExecuteMacro(RS_INFO->sum_dir, hinfo.macroString);
			}else{
				if(hdef->sum_hist) hdef->sum_hist->Draw();
			}
			
			// Update canvas and save to PNG file
			c1->Update();
			const char *dir = ".";
			cout << "    - Writing plot " << ipage << " of " << HNAMEPATHS.size() << endl;
			char fname[256];
			sprintf(fname,"%s/rs_page%02d.png", dir, ipage);
			c1->SaveAs(fname, "");
			ipage++;
		}

		RS_INFO->Unlock();
	}
	
	

	// Clean up
	delete RS_CMSG;
	delete RS_INFO;

	cout << "Done" << endl;

	return 0;
}

//-----------
// GetHists
//-----------
bool GetHists(const set<string> &hnamepaths, uint32_t timeout_secs)
{
	// Send requests for all of the listed hnamepaths and
	// then wait until at least one server responds with each.
	// A check is first made that all histogram definitons are
	// already present and this will wait up to timeout_secs 
	// for them to come in. (They are all requested at the beginning
	// of the program.)
	// 
	// Once all definitions are present, requests for the hnamepaths
	// themselves are sent. A minimum of 2 seconds is spent waiting
	// in order to  give multiple servers a chance to respond. A maximum
	// of timeout_secs is spent waiting. This may also spend less than
	// 2 seconds waiting if the received times of all relevant histograms
	// indicate a request was sent out at least 2 seconds earlier.
	// If all hists are found then true is returned. Otherwise false is
	// returned.

	// ------------- Wait for histogram definitions --------------
	cout << "Checking for histogram definitions ..." << endl;
	int Ntries = 0;
	while(!DONE){

		// Check received hdefs
		set<string> found_hnamepaths;
		RS_INFO->Lock();
		for(auto hdef : RS_INFO->histdefs){
			for(string s : HNAMEPATHS) if( s == hdef.second.hnamepath ) found_hnamepaths.insert(s);
		}
		RS_INFO->Unlock();

		// Update ticker and finish if appropriate
		cout << "  Received " << found_hnamepaths.size() << " / " << HNAMEPATHS.size() << " definitions  \r";
		cout .flush();
		if(found_hnamepaths.size() == HNAMEPATHS.size()) break;
		useconds_t sleep_tics = 200000;
		usleep(sleep_tics);
		if((Ntries++)*sleep_tics/1000 > timeout_secs*1000) {
			cerr << endl << "Timed out waiting for hnamepath definitions!" << endl;
			return false;
		}
	}

	// ------------- Request all hnamepaths --------------
	// Request all hnamepaths
	cout << "Requesting hnamepaths:" << endl;
	for(string s : HNAMEPATHS){
		cout << "   " << s << endl;
		RS_CMSG->RequestHistogram("rootspy", s);
	}

	// ------------- Wait for histograms --------------
	cout << endl << "Waiting for histograms .... " << endl;
	Ntries = 0;
	while(!DONE){
	
		// current time in seconds 
		double now = RS_CMSG->GetTime();

		// Register any ROOT items or macros queued up by rs_cmsg
		RegisterQueuedItems();

		// Check received hists
		set<string> found_hnamepaths;
		uint32_t Nhists = 0; // total histograms recieved
		double youngest; // the hdef whose oldest received histogram is yougest 
		RS_INFO->Lock();
		for(auto hdef : RS_INFO->histdefs){
			for(string s : HNAMEPATHS){
				if( s == hdef.second.hnamepath ){
					if( !hdef.second.hists.empty() ){
						found_hnamepaths.insert(s);
						Nhists += hdef.second.hists.size();
						double oldest = now;
						for(auto hinfopair : hdef.second.hists){
							double received = hinfopair.second.received;
							if(received < oldest) oldest = received;
						}
						
						// This keeps track of the hdef whose oldest received
						// histogram is the youngest of all hdefs.						
						if( oldest > youngest ) youngest = oldest;
					}
				}
			}
		}
		RS_INFO->Unlock();

		// Update ticker
		cout << "  Received " << found_hnamepaths.size() << " / " << HNAMEPATHS.size() << "  (" << Nhists << " hists)  \r";
		cout .flush();

		uint32_t sleep_tics = 200000;
		uint32_t min_tries = (2*1000000)/sleep_tics; // 2 seconds
		uint32_t max_tries = (timeout_secs*1000000)/sleep_tics; // 2 seconds
				
		// Check if everything is found
		if(found_hnamepaths.size() == HNAMEPATHS.size()) {
			if(Ntries >= min_tries)break;
			
			// This allows us to break earlier than the minimum time if we see that
			// all hnamepaths have been found and all have at least one received
			// histogram that is the minimum age old. The idea is that we must have
			// already requested that hnamepath that long ago so there is no need to
			// wait an additional min age.
			if( (now-youngest) >=  (double)min_tries*(double)sleep_tics/1.0E6) break;
		}

		// Check if past timeout
		if(Ntries >= max_tries) {
			cerr << endl << "Timed out waiting for hists!" << endl;
			return false;
		}

		// sleep for a cycle
		Ntries++;
		usleep(sleep_tics);
	}
	
	return true;
}

//-----------
// RegisterQueuedItems
//-----------
void RegisterQueuedItems(void)
{
	// This was copied from rs_mainframe::DoTimer in the RootSpy GUI program.
	// That tries to funnel all ROOT operations through the main GUI thread
	// so in order to actually create ROOT objects, we must process the queues
	// here.


	// Register any histograms waiting in the queue
	if( ! HISTOS_TO_REGISTER.empty() ){
		REGISTRATION_MUTEX.lock();
		for(auto h : HISTOS_TO_REGISTER){
			RS_CMSG->RegisterHistogram(h.second, (cMsgMessage*)h.first, true);
		}
		HISTOS_TO_REGISTER.clear();
		REGISTRATION_MUTEX.unlock();
	}
	
	// Register any macros waiting in the queue
	if( ! MACROS_TO_REGISTER.empty() ){
		REGISTRATION_MUTEX.lock();
		for(auto m : MACROS_TO_REGISTER){
			RS_CMSG->RegisterMacro(m.second, (cMsgMessage*)m.first);
		}
		MACROS_TO_REGISTER.clear();
		REGISTRATION_MUTEX.unlock();
	}
}

//-------------------
// ExecuteMacro
//-------------------
void ExecuteMacro(TDirectory *f, string macro)
{
	// Copied from rs_mainframe.cc and modified for here.

	// Lock ROOT
	pthread_rwlock_wrlock(ROOT_MUTEX);

	TStyle savestyle(*gStyle);

	TDirectory *savedir = gDirectory;
	f->cd();

	// execute script line-by-line
	// maybe we should do some sort of sanity check first?
	istringstream macro_stream(macro);
	int iline = 0;
	while(!macro_stream.eof()) {
		string s;
		getline(macro_stream, s);
		iline++;
		
		// This macro should have been parsed upon receipt and all histograms
		// already requested and received based on special comments. Thus,
		// we do not need to request them here.
		
		// Process line
		Long_t err = gROOT->ProcessLine(s.c_str());
		if(err == TInterpreter::kProcessing){
			cout << "Processing macro ..." << endl;
		}else  if(err != TInterpreter::kNoError){
			cout << "Error processing the macro line " << iline << ": " << err << endl;
			cout << "\"" << s << "\"" << endl;
			break;
		}
	}

	// restore
	savedir->cd();
	
	*gStyle = savestyle;

	// Unlock ROOT
	pthread_rwlock_unlock(ROOT_MUTEX);

}

//-----------
// ParseCommandLineArguments
//-----------
void ParseCommandLineArguments(int &narg, char *argv[])
{
	if(VERBOSE>1) _DBG_ << "In ParseCommandLineArguments().." << endl;
		
	// allow for environment variables
	const char *ptr = getenv("ROOTSPY_UDL");
	if(ptr)ROOTSPY_UDL = ptr;

	// check command line options
	static struct option long_options[] = {
		{"help",           no_argument,       0,  'h' },
		{"run-number",     required_argument, 0,  'R' },
		{"udl",            required_argument, 0,  'u' },
		{"server",         required_argument, 0,  's' },
		{"hnamepath",      required_argument, 0,  'H' },
		{"verbose",        no_argument,       0,  'V' },
		{0, 0, 0, 0  }
	};
    
	int opt = 0;
	int long_index = 0;
	while ((opt = getopt_long(narg, argv,"hR:u:s:H:v", long_options, &long_index )) != -1) {
		switch (opt) {
			case 'R':
				if(optarg == NULL) Usage();
				RUN_NUMBER = atoi(optarg);
			case 'u' :
				if(optarg == NULL) Usage();
				ROOTSPY_UDL = optarg;
				break;
			case 's' :
				if(optarg == NULL) Usage();
				ROOTSPY_UDL = "cMsg://";
				ROOTSPY_UDL += optarg;
				ROOTSPY_UDL += "/cMsg/rootspy";
				break;
			case 'v' :
				VERBOSE++;
				break;
			case 'h':
				Usage();
				break;
			case 'H':
				HNAMEPATHS.insert(optarg);
				break;
		}
	}
	
	if(HNAMEPATHS.empty()){
		cout << endl << "You must specify at leat one hnamepath!" << endl << endl;
		Usage();
	}

    // DUMP configuration
    cout << "-------------------------------------------------" << endl;
    cout << "Current configuration:" << endl;
    cout << "ROOTSPY_UDL = " << ROOTSPY_UDL << endl;
    cout << "RUN_NUMBER = " << RUN_NUMBER << endl;
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
	cout<<"   -H,--hnamepath hnamepath  An hnamepath to include (can be multiple of these)" << endl;
	cout<<"   -v,--verbose              Increase verbosity (can use multiple times)"<<endl;
	cout<<endl;
	cout<<endl;

	exit(0);
}

