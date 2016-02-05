
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <time.h>
#include <pthread.h>
#include <sstream>
using namespace std;

#include <RootSpy.h>
#include <hdef_t.h>
#include <hinfo_t.h>
#include <rs_cmsg.h>
#include <rs_info.h>

#include "rsmon_cmsg.h"


#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TRandom.h>
#include <TTree.h>
#include <TFile.h>

bool DONE =false;
uint32_t DELAY=400000; // in microseconds
string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
string CMSG_NAME;
int VERBOSE=0;
bool REDRAW_SCREEN = true; // for debugging
string FOCUS_NODE = "";
bool INCLUDE_ROOTSPY_STATS = true;
bool USE_RS_CMSG=false;
bool PING_SERVERS = false;
bool RESPOND_TO_PINGS = true;
int REQUESTS_SENT=0;
int REQUESTS_SATISFIED=0;
double START_TIME = 0;
map<hid_t, double> LAST_REQUEST_TIME;

enum RSMON_MODE{
	MODE_OBSERVE,
	MODE_SPEED_TEST,
	MODE_MESS_SIZES
};

rsmon_cmsg *RSMON_CMSG = NULL;
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;
RSMON_MODE MODE = MODE_OBSERVE;

void Usage(void);
void ParseCommandLineArguments(int narg, char *argv[]);
void sigHandler(int sig) { DONE = true; }
void UpdateHistoRequests(double now);


#define ESC (char)0x1B
//#define ESC endl<<"ESC"
#define CLEAR()         {cout<<ESC<<"[2J";}
#define HOME()          {cout<<ESC<<"[;H";}
#define MOVETO(X,Y)     {cout<<ESC<<"["<<Y<<";"<<X<<"H";}
#define PRINTAT(X,Y,S)  {MOVETO(X,Y);cout<<S;}
#define PRINTCENTERED(Y,S) {MOVETO((Ncols-string(S).length())/2,Y);cout<<S;}
#define REVERSE()       {cout<<ESC<<"[7m";}
#define RESET()         {cout<<ESC<<"[0m";}
#define BOLD()         {cout<<ESC<<"[1m";}

//------------------------------
// PrintToString
//------------------------------
string PrintToString(hinfo_t &hinfo, hdef_t &hdef)
{
	stringstream ss;
	int spaces = 25 - hinfo.serverName.length();
	if(spaces<1) spaces = 1;
	ss << string(spaces, ' ');
	ss << hinfo.serverName;
	
	ss << " ";
	
	spaces = 20 - hinfo.hnamepath.length();
	if(spaces<1) spaces = 1;
	ss << string(spaces, ' ');
	ss << hinfo.hnamepath;
	
	ss << " ";
	
	string type = "<none>";
	switch(hdef.type){
		case hdef_t::noneD: type = "??"; break;
		case hdef_t::oneD: type = "TH1D"; break;
		case hdef_t::twoD: type = "TH2D"; break;
		case hdef_t::threeD: type = "TH3D"; break;
		case hdef_t::profile: type = "TProfile"; break;
		case hdef_t::macro: type = "Macro"; break;
	}
	spaces = 10 - type.length();
	if(spaces<1) spaces = 1;
	ss << string(spaces, ' ');
	ss << type;
	
	ss << " ";
	
	char rate_str[32];
	sprintf(rate_str, "%2.2fHz", hinfo.rate);
	spaces = 8 - strlen(rate_str);
	if(spaces<1) spaces = 1;
	ss << string(spaces, ' ');
	ss << rate_str;

	

	return ss.str();
}

//------------------------------
// RedrawScreen
//------------------------------
void RedrawScreen(vector<string> &lines, uint32_t Nhdefs, uint32_t Nhinfos, uint32_t Ntdefs)
{
	if(!REDRAW_SCREEN){   // for debugging
		cout << "---------- Skipping screen redraw of " << lines.size() << " content lines ----------" << endl;
		return;
	}
	// Get size of terminal
	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	int Nrows = w.ws_row;
	int Ncols = w.ws_col;

	HOME();
	CLEAR();
	RESET();
	PRINTAT(1, 1, string(Ncols,'-'));
	PRINTCENTERED(2, "ROOTSpy Monitor");
	PRINTAT(1, 3, string(Ncols,'-'));
	
	string mode_str = "UNKNOWN";
	switch(MODE){
		case MODE_OBSERVE   :  mode_str="OBSERVE ONLY" ;    break;
		case MODE_SPEED_TEST:  mode_str="SPEED TEST"   ;    break;
		case MODE_MESS_SIZES:  mode_str="MESSAGE SIZES";    break;
	}

	MOVETO( 3, 4); cout << " Number of histograms published: " << Nhdefs;
	MOVETO( 3, 5); cout << "Number of histograms downloaded: " << Nhinfos;
	MOVETO( 3, 6); cout << "      Number of trees published: " << Ntdefs;
	MOVETO(41, 4); cout << "Number of requests   issued: " << REQUESTS_SENT;
	MOVETO(41, 5); cout << "Number of requests received: " << REQUESTS_SATISFIED;
	MOVETO(41, 6); cout << "MODE: " << mode_str;
	PRINTAT(1, 7, string(Ncols,'.'));

	for(unsigned int i=0; i<lines.size(); i++){
		int row = i+10;
		MOVETO(1, row);
		if(row == (Nrows-1)){ cout << " ..."; break; }
		string &line = lines[i];
		if(line.find("BOLD")==0){
			BOLD();
			cout << line.substr(4);
			RESET();
		}else{
			cout << line;
		}
	}
	
	// Print current time at bottom
	time_t current_time = time(NULL);
	string current_time_str = ctime(&current_time);
	current_time_str.erase(current_time_str.length()-1);
	REVERSE();
	PRINTAT(1, Nrows, string(Ncols,' ')); // black strip
	PRINTAT(1, Nrows, current_time_str);
	

	MOVETO(Ncols, Nrows);
	RESET();
	cout.flush();
}

//------------------------------
// Usage
//------------------------------
void Usage(void)
{
	string str=""
	" Usage:\n"
	"     RSMonitor [options]\n"
	"\n"
	"Retreive all histograms from available RootSpy\n"
	"servers and print list of them to screen along\n"
	"with retrieval statistics.\n"
	"\n"
	"  options:\n"
	"\n"
	" -h,--help  Print this usage statement\n"
	" -v verbose Set verbosity level (def. is 0)\n"
	" -udl UDL   Set the RootSpy UDL for connecting\n"
	"            to the cMsg server.\n"
	" -s,--speed Run in speed test mode where all histograms\n"
	"            from all processes are grabbed as quickly as\n"
	"            possible to system capabilities.\n"
	" --sizes    Run in message size mode where the size of messages\n"
	"            is shown (in decreasing order)\n"
	" -f focus   Set the \"focus\" node. This should be\n"
	"            the rootspy name of the process you wish\n"
	"            to focus on. Information on other processes\n"
	"            will not be printed while more details of the\n"
	"            focus process will. This only affects normal\n"
	"            mode and not speed test mode.\n"
	" -p         Ping servers (default is to not ping servers but\n"
	"            to listen for responses to others' pings\n"
	" --stealth  Operate in stealth mode. Basically, do not respond\n"
	"            to pings. This has not effect if in speed test mode\n"
	"            and does not ensure complete stealthy-ness since\n"
	"            one can still specfiy \"-p\"\n"
	" -delay us  Set the minimum delay between\n"
	"            requests for a histogram. Value is\n"
	"            in microseconds. (See not below)\n"
	" --norootspy Do not include stats for commands sent to the\n"
	"            generic \"rootspy\" subject in reported stats.\n"
	" --noredraw Do not redraw the screen (only useful\n"
	"            for debugging to allow print statments\n"
	"            to show up.\n"
	"\n"
	"If the -udl option is not given, the value is\n"
	"taken from the ROOTSPY_UDL environment variable.\n"
	"If that is not set, then the default value of\n"
	"  cMsg://127.0.0.1/cMsg/rootspy  is used.\n"
	"\n"
	"The value of the -delay option is used as a sleep\n"
	"time between rounds of requesting histograms.\n"
	"Thus, a histogram will not be requested more\n"
	"frequently than once every \"delay\" microseconds.\n"
	"Histogram requests are only sent out when a histo\n"
	"is first declared by the server (via a response\n"
	"to a \"list hists\" request) or after the histogram\n"
	"has been received. Thus, the servers are never\n"
	"flooded with requests at a higher rate than they\n"
	"can keep up with. Note that the screen updates at\n"
	"1Hz and is independent of the value of delay.\n"
	"\n"
	"Rates displayed are calculated simply from the\n"
	"difference in times between the last two times\n"
	"a histogram was received. It is measured by a\n"
	"millisecond level clock so is pretty inaccurate\n"
	"for rates approaching 1kHz.\n"
	"\n";

	cout << str << endl;
	exit(0);
}

//------------------------------
// ParseCommandLineArguments
//------------------------------
void ParseCommandLineArguments(int narg, char *argv[])
{
	const char *ptr = getenv("ROOTSPY_UDL");
	if(ptr)ROOTSPY_UDL = ptr;

	for(int iarg=1; iarg<narg; iarg++){
		string arg = argv[iarg];
		string next = (iarg+1)<narg ? argv[iarg+1]:"";
		
		bool needs_arg = false;

		     if(arg=="-h"     || arg=="--help"    )  Usage();
		else if(arg=="-v"     || arg=="--verbose" ) {VERBOSE = atoi(next.c_str()); needs_arg=true;}
		else if(arg=="-udl"   || arg=="--udl"     ) {ROOTSPY_UDL = next; needs_arg=true;}
		else if(arg=="-delay" || arg=="--delay"   ) {DELAY = atoi(next.c_str()); needs_arg=true;}
		else if(arg=="-s"     || arg=="--speed"   ) {MODE = MODE_SPEED_TEST;}
		else if(arg=="--sizes"                    ) {MODE = MODE_MESS_SIZES;}
		else if(arg=="-f"     || arg=="--focus"   ) {FOCUS_NODE = next; needs_arg=true;}
		else if(arg=="-p"     || arg=="--ping"    ) {PING_SERVERS = true;}
		else if(arg=="--stealth"                  ) {RESPOND_TO_PINGS = false;}
		else if(arg=="--norootspy"                ) {INCLUDE_ROOTSPY_STATS=false;}
		else if(arg=="--noredraw"                 ) {REDRAW_SCREEN = false;}
		else{
			cout << "Unknown argument: " << arg << endl;
			exit(-2);
		}

		if(needs_arg){
			if((iarg+1>=narg) || next[0]=='-'){
				cout << "Argument \""<<arg<<"\" needs an argument!" << endl;
				exit(-1);
			}
			
			iarg++;
		}
	}
	
	if(MODE == MODE_SPEED_TEST) USE_RS_CMSG = true;
}

//------------------------------
// UpdateHistoRequests
//------------------------------
void UpdateHistoRequests(double now)
{
	// Loop over currently defined histograms, making list of histograms
	// to request because they have been updated since our last request 
	// or have never been updated
	RS_INFO->Lock();
	map<string,hdef_t> &hdefs = RS_INFO->histdefs;
	map<hid_t,hinfo_t> &hinfos = RS_INFO->hinfos;
	vector<hid_t> hists_to_request;
	map<string,hdef_t>::iterator hdef_iter = hdefs.begin();
	for(; hdef_iter!= hdefs.end() ; hdef_iter++){

		hdef_t &hdef = hdef_iter->second;
		string &hnamepath = hdef.hnamepath;

		// Loop over servers who can provide this histogram
		map<string, bool>::iterator it = hdef.servers.begin();
		for(; it!=hdef.servers.end(); it++){
			string server = it->first;

			hid_t hid(server, hnamepath);

			map<hid_t, double>::iterator it_lrt = LAST_REQUEST_TIME.find(hid);
			if(it_lrt != LAST_REQUEST_TIME.end()){

				// Time we last sent a request
				double lrt = it_lrt->second;

				// Look to see if we've received this histogram from this server
				map<hid_t,hinfo_t>::iterator it_hinfo = hinfos.find(hid);
				if(it_hinfo != hinfos.end()){

					double received = it_hinfo->second.received - START_TIME;
					if( lrt<received ){
						REQUESTS_SATISFIED++;
					}else{
						// Don't send second request within 2 seconds of previous
						if((now-lrt)<2.0) continue;

						// Stop requesting histograms from non-responsive servers
						if((now-received)>10.0 ) continue;
					}
				}else{
					// If we've never received this histogram and
					// it has been less than 2 seconds, don't send
					// another request
					if((now-lrt)<2.0) continue;
				}
			}
			hists_to_request.push_back(hid);
		}

	}

	RS_INFO->Unlock();

	// Sort list by server so we can send single request to each
	// server and they can return all histograms in single message
	map<string, vector<string> > hnamepaths_by_server;
	for(unsigned int i=0; i<hists_to_request.size(); i++){
		string &server = hists_to_request[i].serverName;
		string &hnamepath = hists_to_request[i].hnamepath;
		hnamepaths_by_server[server].push_back(hnamepath);
	}
	map<string, vector<string> >::iterator hbs_iter = hnamepaths_by_server.begin();
	for(; hbs_iter!=hnamepaths_by_server.end(); hbs_iter++){
		string server = hbs_iter->first;
		vector<string> &hnamepaths = hbs_iter->second;
		RS_CMSG->RequestHistograms(server, hnamepaths);
		for(uint32_t i=0; i<hnamepaths.size(); i++){
			LAST_REQUEST_TIME[hid_t(server, hnamepaths[i])] = now;
			REQUESTS_SENT++;
		}
	}

#if 0

	// Request histograms outside of mutex lock
	for(unsigned int i=0; i<hists_to_request.size(); i++){
		string &server = hists_to_request[i].serverName;
		string &hnamepath = hists_to_request[i].hnamepath;
		RS_CMSG->RequestHistogram(server, hnamepath);
		LAST_REQUEST_TIME[hid_t(server, hnamepath)] = now;
		REQUESTS_SENT++;
	}
#endif
}

//------------------------------
// main
//------------------------------
int main(int narg, char *argv[])
{

	ParseCommandLineArguments(narg, argv);


	// Create rs_info object
	RS_INFO = new rs_info();
	
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);

	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "rs_%s_%d", hostname, getpid());
	CMSG_NAME = string(str);
	cout << "Full UDL is " << ROOTSPY_UDL << endl;
	if(USE_RS_CMSG){
		RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
		RS_CMSG->verbose=VERBOSE;
		RS_CMSG->program_name = "RSMonitor";
	}

	signal(SIGINT, sigHandler);
	
	// Loop forever Getting all Histograms
	START_TIME = rs_cmsg::GetTime();
	double next_update = 1.0; // time relative to start_time
	while(!DONE){

		// Get Current time
		double now = rs_cmsg::GetTime() - START_TIME; // measure time relative to program start
		
		switch(MODE){
			case MODE_OBSERVE:
			case MODE_MESS_SIZES:
				if(RSMON_CMSG==NULL && (RS_CMSG==NULL || RS_CMSG->IsOnline())){
					RSMON_CMSG = new rsmon_cmsg(CMSG_NAME, (RS_CMSG==NULL ? NULL:RS_CMSG->GetcMsgPtr()));
					RSMON_CMSG->focus_node = FOCUS_NODE;
					RSMON_CMSG->respond_to_pings = RESPOND_TO_PINGS;
				}
				break;
			case MODE_SPEED_TEST:
				UpdateHistoRequests(now);
				break;
		}
		

		// Update screen occasionally
		if( now >= next_update ){
		
			// Container to hold lines to print in main content area of screen
			vector<string> lines;

			// Fill in lines based on what mode we're running in
			switch(MODE){
				case MODE_OBSERVE:
					if(RSMON_CMSG) RSMON_CMSG->FillLines(now, lines);
					break;
				case MODE_MESS_SIZES:
					if(RSMON_CMSG) RSMON_CMSG->FillLinesMessageSizes(now, lines);
					break;
				case MODE_SPEED_TEST:
					RS_INFO->Lock();			
					map<hid_t,hinfo_t> &hinfos = RS_INFO->hinfos;
					for(map<hid_t,hinfo_t>::iterator iter=hinfos.begin(); iter!=hinfos.end(); iter++){
						hinfo_t &hinfo = iter->second;
						if(now + START_TIME - hinfo.received > 4.0) continue; // don't print lines for hists that are no longer being updated
						hdef_t &hdef = RS_INFO->histdefs[hinfo.hnamepath];
						string line = PrintToString(hinfo, hdef);
						lines.push_back(line);
					}		
					RS_INFO->Unlock();
					break;
			}
			
			// Get some statistics to display
			RS_INFO->Lock();
			uint32_t Nhdefs  = RS_INFO->histdefs.size();
			uint32_t Nhinfos = RS_INFO->hinfos.size();
			uint32_t Ntdefs  = RS_INFO->treedefs.size();
			RS_INFO->Unlock();

			// Redraw the screen
			RedrawScreen(lines, Nhdefs, Nhinfos, Ntdefs);
			
			// Do any follow-up based on mode
			switch(MODE){
				case MODE_OBSERVE:
				case MODE_MESS_SIZES:
					if(RS_CMSG) {
						if(PING_SERVERS) RS_CMSG->PingServers();
					} else {
						if(PING_SERVERS) RSMON_CMSG->PingServers();
					}
					break;
				case MODE_SPEED_TEST:
					// Request new histogram list from all servers
					if(RS_CMSG) RS_CMSG->RequestHists("rootspy");

					// Request new tree list from all servers
					if(RS_CMSG) RS_CMSG->RequestTreeInfo("rootspy");
					break;
			}

			next_update = now + 1.0; // update again in 1s
		}		

		// sleep a bit in order to limit how fast the histos are filled
		if(DELAY != 0)usleep(DELAY);
	}

	cout << endl;
	cout << "Ending" << endl;
	
	if(RSMON_CMSG) delete RSMON_CMSG;
	if(RS_CMSG) delete RS_CMSG;

	return 0;
}



