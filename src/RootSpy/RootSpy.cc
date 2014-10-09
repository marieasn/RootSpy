#include <unistd.h>
#include <pthread.h>
#include <TGApplication.h>

#include "RootSpy.h"
#include "rs_mainframe.h"
//#include "rs_mainframe_multi.h"
#include "rs_cmsg.h"
#include "rs_info.h"

// GLOBALS
rs_mainframe *RSMF = NULL;
rs_cmsg *RS_CMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;

string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
string CMSG_NAME = "<not set here. see below>";

bool MULTIFRAME_MODE = false;

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);

//-------------------
// main
//-------------------
int main(int narg, char *argv[])
{
	// Parse the command line arguments
	ParseCommandLineArguments(narg, argv);

	// Create rs_info object
	RS_INFO = new rs_info();

	// Create a ROOT TApplication object
	TApplication app("RootSpy", &narg, argv);

	// Makes a Mutex to lock / unlock Root Global
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);

	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "RootSpy GUI %s-%d", hostname, getpid());
	CMSG_NAME = string(str);
	cout << "Full UDL is " << ROOTSPY_UDL << endl;
	RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
	RS_CMSG->verbose = 1;
	
	const char *ROOTSPY_VERBOSE = getenv("ROOTSPY_VERBOSE");
	if(ROOTSPY_VERBOSE) RS_CMSG->verbose = atoi(ROOTSPY_VERBOSE);


	//cout<<__FILE__<<__LINE__<<" "<<gClient->ClassName();

	// Create the GUI window
	//if(MULTIFRAME_MODE) {
	//  RSMF = new rs_mainframe_multi(gClient->GetRoot(), 10, 10, true);
	//} else
	RSMF = new rs_mainframe(gClient->GetRoot(), 10, 10, true);
	
	// Hand control to the ROOT "event" loop
	app.SetReturnFromRun(true);
	app.Run();
	
	delete RS_CMSG;
	delete RSMF;

	return 0;
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

	// Loop over comman line arguments
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
			case 's':
				if(i>=(narg-1)){
					cerr<<"-s option requires an argument"<<endl;
				}else{
					ROOTSPY_UDL = "cMsg://";
					ROOTSPY_UDL += argv[i+1];
					ROOTSPY_UDL += "/cMsg/rootspy";
				}
				break;
		        case 'm':
			    if(strcmp("-multi",argv[i])==0) { 
				MULTIFRAME_MODE = true;
				if(i>=(narg-1)){
				    cerr<<"-multi option requires an argument"<<endl;
				}else{
				    ;  // load filename
				}
				break;
			    }
		}
	}
}

//-----------
// Usage
//-----------
void Usage(void)
{
	cout<<"Usage:"<<endl;
	cout<<"       RootSpy [options]"<<endl;
	cout<<endl;
	cout<<"Connect to programs with the rootspy plugin attached"<<endl;
	cout<<"and spy on thier histograms."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h        Print this message"<<endl;
	cout<<"   -u udl    UDL of cMsg server(def. "<<ROOTSPY_UDL<<")"<<endl;
	cout<<"   -n name   Specify name this program registers with cMsg server"<<endl;
	cout<<"             (def. "<<CMSG_NAME<<")"<<endl;
	//cout<<""; // multimode
	cout<<endl;

	exit(0);
}
