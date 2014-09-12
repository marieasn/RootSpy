
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


#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TRandom.h>
#include <TTree.h>
#include <TFile.h>

bool DONE =false;
uint32_t DELAY=1000; // in microseconds
string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";


rs_mainframe *RSMF;
rs_cmsg *RS_CMSG;
rs_info *RS_INFO;
pthread_rwlock_t *ROOT_MUTEX;


void sigHandler(int sig) { DONE = true; }

#define ESC (char)0x1B
//#define ESC endl<<"ESC"
#define CLEAR()         {cout<<ESC<<"[2J";}
#define HOME()          {cout<<ESC<<"[;H";}
#define MOVETO(X,Y)     {cout<<ESC<<"["<<Y<<";"<<X<<"H";}
#define PRINTAT(X,Y,S)  {MOVETO(X,Y);cout<<S;}
#define PRINTCENTERED(Y,S) {MOVETO((Ncols-string(S).length())/2,Y);cout<<S;}
#define REVERSE()       {cout<<ESC<<"[7m";}
#define RESET()         {cout<<ESC<<"[0m";}

//------------------------------
// PrintToString
//------------------------------
string PrintToString(hinfo_t &hinfo, hdef_t &hdef)
{
	stringstream ss;
	int spaces = 17 - hinfo.serverName.length();
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
void RedrawScreen(vector<string> &lines, uint32_t Nhdefs, uint32_t Nhinfos)
{
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

	MOVETO(3, 4); cout << "Number of histograms  published: " << Nhdefs;
	MOVETO(3, 5); cout << "Number of histograms downloaded: " << Nhinfos;
	PRINTAT(1, 6, string(Ncols,'.'));

	for(unsigned int i=0; i<lines.size(); i++){
		int row = i+9;
		MOVETO(1, i+9);
		if(row == (Nrows-1)){ cout << " ..."; break; }
		cout << lines[i];
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
// main
//------------------------------
int main(int narg, char *argv[])
{

	const char *ptr = getenv("ROOTSPY_UDL");
	if(ptr)ROOTSPY_UDL = ptr;

	// Create rs_info object
	RS_INFO = new rs_info();
	
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);

	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "rootspy_consumer_%s-%d", hostname, getpid());
	string CMSG_NAME = string(str);
	cout << "Full UDL is " << ROOTSPY_UDL << endl;
	RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
	RS_CMSG->verbose=1;

	signal(SIGINT, sigHandler);
	
	struct timeval last_tval;
	struct timezone tzone;
	gettimeofday(&last_tval, &tzone);
	
	// Loop forever Getting all Histograms
	double start_time = rs_cmsg::GetTimeMS();
	double next_update = 1.0; // time relative to start_time
	map<hid_t, double> last_request_time;
	while(!DONE){

		// Get Current time
		double now = rs_cmsg::GetTimeMS() - start_time; // measure time relative to program start
		
		// Loop over currently defined histograms, making list of histograms
		// to request because they have been updated since our last request 
		// or have never been updated
		RS_INFO->Lock();
		map<string,hdef_t> &hdefs = RS_INFO->histdefs;
		map<hid_t,hinfo_t> &hinfos = RS_INFO->hinfos;
		uint32_t Nhdefs = hdefs.size();
		uint32_t Nhinfos = hinfos.size();
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

				map<hid_t, double>::iterator it_lrt = last_request_time.find(hid);
				if(it_lrt != last_request_time.end()){
					
					// Look to see if we've received this histogram from this server
					map<hid_t,hinfo_t>::iterator it_hinfo = hinfos.find(hid);
					if(it_hinfo != hinfos.end()){
						double lrt = it_lrt->second;
						double received = it_hinfo->second.received;
						if( (lrt>received) && ((now-lrt)<2.0)) continue; // Have not received response since our last request
					}
				}
				hists_to_request.push_back(hid);
			}
			
		}
		RS_INFO->Unlock();
		
		// Request histograms outside of mutex lock
		for(unsigned int i=0; i<hists_to_request.size(); i++){
			string &server = hists_to_request[i].serverName;
			string &hnamepath = hists_to_request[i].hnamepath;
			RS_CMSG->RequestHistogram(server, hnamepath);
			last_request_time[hid_t(server, hnamepath)] = now;
		}

		// Update screen occasionally
		if( now >= next_update ){
			RS_INFO->Lock();
			
			vector<string> lines;
			map<hid_t,hinfo_t> &hinfos = RS_INFO->hinfos;
			for(map<hid_t,hinfo_t>::iterator iter=hinfos.begin(); iter!=hinfos.end(); iter++){
				hinfo_t &hinfo = iter->second;
				if(now + start_time - hinfo.received > 2.0) continue; // don't print lines for hists that are no longer being updated
				hdef_t &hdef = RS_INFO->histdefs[hinfo.hnamepath];
				string line = PrintToString(hinfo, hdef);
				lines.push_back(line);
			}
			
			RS_INFO->Unlock();
			
			RedrawScreen(lines, Nhdefs, Nhinfos);
			
			// Request new histogram list from all servers
			RS_CMSG->RequestHists("rootspy");

			next_update = now + 1.0; // update again in 1s
		}		

		// sleep a bit in order to limit how fast the histos are filled
		if(DELAY != 0)usleep(DELAY);
	}

	cout << endl;
	cout << "Ending" << endl;

	return 0;
}
