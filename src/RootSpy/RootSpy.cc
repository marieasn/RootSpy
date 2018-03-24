
#include <fstream>
using namespace std;

#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <TGApplication.h>

#include "RootSpy.h"
#include "rs_mainframe.h"
#include "rs_cmsg.h"
#include "rs_xmsg.h"
#include "rs_info.h"

// GLOBALS
rs_mainframe *RSMF = NULL;
rs_cmsg *RS_CMSG = NULL;
rs_xmsg *RS_XMSG = NULL;
rs_info *RS_INFO = NULL;
pthread_rwlock_t *ROOT_MUTEX = NULL;
int VERBOSE = 1;

bool USE_CMSG=false;
bool USE_XMSG=true;

string ROOTSPY_UDL = "cMsg://127.0.0.1/cMsg/rootspy";
string CMSG_NAME = "<not set here. see below>";

string ELOG_NAME   = "HDMONITOR";
string ELOG_EMAIL  = "nsj@cmu.edu";
bool   ELOG_NOTIFY = true;

string CONFIG_FILENAME = "";
string INPUT_ROOT_FILENAME = "";
string PLUGINS_STR = "";

void ParseCommandLineArguments(int &narg, char *argv[]);
void Usage(void);
void AttachPlugins(void);
void* FailedAttachHack(string fullpath, string err);

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
	TApplication *app = new TApplication("RootSpy", &narg, argv);

	// Makes a Mutex to lock / unlock Root Global
	ROOT_MUTEX = new pthread_rwlock_t;
	pthread_rwlock_init(ROOT_MUTEX, NULL);

	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "RootSpy GUI %s-%d", hostname, getpid());
	CMSG_NAME = string(str);
	const char *ROOTSPY_VERBOSE = getenv("ROOTSPY_VERBOSE");
	if(ROOTSPY_VERBOSE) VERBOSE = atoi(ROOTSPY_VERBOSE);
	cout << "Full UDL is " << ROOTSPY_UDL << endl;
	cout << "Setting verbosity level to " << VERBOSE <<endl;
	if( USE_CMSG ){
		RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME, INPUT_ROOT_FILENAME=="");
		RS_CMSG->verbose = VERBOSE;
		RS_CMSG->hist_default_active = false;
		RS_CMSG->program_name = "RootSpy";
		RS_CMSG->verbose = VERBOSE;
	}
	if( USE_XMSG ){
		RS_XMSG = new rs_xmsg(ROOTSPY_UDL, CMSG_NAME, INPUT_ROOT_FILENAME=="");
		RS_XMSG->verbose = VERBOSE;
		RS_XMSG->hist_default_active = false;
		RS_XMSG->program_name = "RootSpy";
		RS_XMSG->verbose = VERBOSE;
	}

	if(INPUT_ROOT_FILENAME != "") RS_INFO->LoadRootFile(INPUT_ROOT_FILENAME);

	AttachPlugins();

	//cout<<__FILE__<<__LINE__<<" "<<gClient->ClassName();

	// Create the GUI window
	RSMF = new rs_mainframe(gClient->GetRoot(), 10, 10, true, CONFIG_FILENAME);
	
	// Hand control to the ROOT "event" loop
	app->Run(kTRUE);
	
	if(RS_CMSG) delete RS_CMSG;
	if(RSMF) delete RSMF;

	// I recall spending some time tearing my hair out over
	// this in the past. Currently though, it will seg. fault
	// in the online system (RHEL6.4 + ROOT 5.34) if the TApplication
	// is deleted. Until recently, the TApplication was allocated
	// on the stack so it was automatically getting deleted. This
	// seemed to work since SetReturnFromTrue() was being called before
	// Run() and Run() was not passed any arguments. This caused it
	// to set the ReturnFromRun flag to false so the program terminated
	// without returning from TApplication::Run(). This was bad since
	// the rs_cmsg object was not being deleted, therefore preventing
	// its destructor from being called, therefore denying it the
	// opportunity to write out a debug file. Phew! I know that is
	// long winded, but wanted to leave a note to myself reminding
	// me of the ramifications if this were changed. Also, because
	// this seems to be a somewhat workable solution in the counting
	// house, I don't want this changed to make it work on other
	// platforms unless it is fully tested on the counting house 
	// system too!
	//delete app;

	// This is here to force the rs_influxdb class and the global
	// InsertSeriesData() routines to be linked into the RootSpy
	// executable. Without this, they won't be pulled in by the
	// linker and therefore won't be available to the macros
	if(narg<0){
		void InitSeriesData(void);
		InitSeriesData();
	}

	_exit(0);  // Skip ROOT's installed cleanups and thereby end of program seg. faults.


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
			case 'v':
				VERBOSE++;
				break;
			case 'u':
				if(i>=(narg-1)){
					cerr<<"-u option requires an argument"<<endl;
					Usage();
				}else{
					ROOTSPY_UDL = argv[i+1];
				}
				break;
			case 'n':
				if(i>=(narg-1)){
					cerr<<"-n option requires an argument"<<endl;
					Usage();
				}else{
					CMSG_NAME = argv[i+1];
				}
				break;
			case 's':
				if(i>=(narg-1)){
					cerr<<"-s option requires an argument"<<endl;
					Usage();
				}else{
					ROOTSPY_UDL = "cMsg://";
					ROOTSPY_UDL += argv[i+1];
					ROOTSPY_UDL += "/cMsg/rootspy";
				}
				break;
			case 'c':
				if(i>=(narg-1)){
					cerr<<"-c option requires an argument"<<endl;
					Usage();
				}else{					
					CONFIG_FILENAME = argv[i+1];
					if(CONFIG_FILENAME.length() > 0) {
						char *buf = NULL;
						// make sure we have an absolute pathname
						CONFIG_FILENAME = getcwd(buf,0) + string("/") + CONFIG_FILENAME;
						free(buf);
					}
				}
				break;
			case 'f':
				if(i>=(narg-1)){
					cerr<<"-f option requires an argument"<<endl;
					Usage();
				}else{
					INPUT_ROOT_FILENAME = argv[i+1];
				}
				break;
			case 'p':
				if(i>=(narg-1)){
					cerr<<"-p option requires an argument"<<endl;
					Usage();
				}else{
					PLUGINS_STR = argv[i+1];
				}
				break;
			case 'L':
				if(i>=(narg-1)){
					cerr<<"-L option requires an argument"<<endl;
					Usage();
				}else{
					ELOG_NAME = argv[i+1];
				}
				break;
			case 'E':
				if(i>=(narg-1)){
					cerr<<"-E option requires an argument"<<endl;
					Usage();
				}else{
					ELOG_EMAIL = argv[i+1];
				}
				break;
			case 'N':
				ELOG_NOTIFY = false;
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
	cout<<"       RootSpy [options]"<<endl;
	cout<<endl;
	cout<<"Connect to programs with the rootspy plugin attached"<<endl;
	cout<<"and spy on their histograms."<<endl;
	cout<<endl;
	cout<<"Options:"<<endl;
	cout<<endl;
	cout<<"   -h          Print this message"<<endl;
	cout<<"   -v          Increase verbosity level"<<endl;
	cout<<"   -c config   Read config specified configuartion file"<<endl;
	cout<<"   -s server   Build UDL using specified cMsg server host"<<endl;
	cout<<"   -u udl      UDL of cMsg server(def. "<<ROOTSPY_UDL<<")"<<endl;
	cout<<"   -n name     Specify name this program registers with cMsg server"<<endl;
	cout<<"               (def. "<<CMSG_NAME<<")"<<endl;
	cout<<"   -p plugins  comma separated list of plugins to attach for macros" << endl;
	cout<<"   -f file     ROOT file to attach. NOTE: if this option is given then" << endl;
	cout<<"               RootSpy will not attach to the cMsg server and only histos" << endl;
	cout<<"               and trees found in the file will be available" << endl;
	cout<<"   -L elog     comma separated list of logbooks to make entries in" << endl;
	cout<<"               specify TLOG for testing (you'll probably also want -N)" << endl;
	cout<<"   -E email    comma separated list of e-mail addresses to send copy of " << endl;
	cout<<"               elog entry to" << endl;
	cout<<"   -N          do NOT send email notification of elog entry" << endl;
	
	//cout<<""; // multimode
	cout<<endl;

	exit(0);
}

//-----------
// AttachPlugins
//-----------
void AttachPlugins(void)
{
	/// This will attach plugins given in the comma separated list
	/// of plugins in the PLUGINS_STR global. This is here so that
	/// plugins that have macros built into them will be able to register
	/// them locally such as when reading the histograms from a local
	/// ROOT file. Simply opening the plugin should cause it to run
	/// the code that will register any macros it has. We do not need to
	/// look for any symbols or run any code explicitly here.
	if(PLUGINS_STR.length()==0) return;

//dlopen("/Users/davidl/HallD/ONLINE/CODA/3.02/Darwin-x86_64/lib/libevio.dylib", RTLD_LAZY| RTLD_GLOBAL | RTLD_NODELETE);
//dlopen("/Users/davidl/HallD/ONLINE/CODA/3.02/Darwin-x86_64/lib/libevioxx.dylib", RTLD_LAZY| RTLD_GLOBAL | RTLD_NODELETE);

	// Split string at commas to get list of plugins
	vector<string> plugins;
	string &str = PLUGINS_STR;
	unsigned int cutAt;
	while( (cutAt = str.find(",")) != (unsigned int)str.npos ){
		if(cutAt > 0)plugins.push_back(str.substr(0,cutAt));
		str = str.substr(cutAt+1);
	}
	if(str.length() > 0)plugins.push_back(str);
	
	// We need to search a set of directories for the plugins. These
	// are based on various environment variables. Get complete list
	// here.
	vector<string> envars; // environment variables to used for search paths
	envars.push_back("ROOTSPY_PLUGIN_PATH");
	envars.push_back("JANA_PLUGIN_PATH");
	envars.push_back("LD_LIBRARY_PATH");

	vector<string> pluginPaths;
	pluginPaths.push_back(".");
	for(unsigned int i=0; i<envars.size(); i++){
		const char *jpp = getenv(envars[i].c_str());
		if(!jpp) continue;

		string str(jpp);
		unsigned int cutAt;
		while( (cutAt = str.find(":")) != (unsigned int)str.npos ){
			if(cutAt > 0) pluginPaths.push_back(str.substr(0,cutAt));
			str = str.substr(cutAt+1);
		}
		if(str.length() > 0) pluginPaths.push_back(str);
	}

	// Loop over plugins, attaching each one we find
	for(unsigned int j=0; j<plugins.size(); j++){
		// Sometimes, the user will include the ".so" suffix in the
		// plugin name. If they don't, then we add it here.
		string plugin = plugins[j];
		if(plugin.substr(plugin.size()-3)!=".so")plugin = plugin+".so";
	
		// Loop over paths
		bool found_plugin=false;
		for(unsigned int i=0; i< pluginPaths.size(); i++){
			string fullpath = pluginPaths[i] + "/" + plugin;
			ifstream f(fullpath.c_str());
			if(f.is_open()){
				f.close();
				
				void* handle = dlopen(fullpath.c_str(), RTLD_LAZY| RTLD_GLOBAL | RTLD_NODELETE);
				if(handle){
					// Successfully attached plugin.
					dlclose(handle);
					cout << "Attached plugin: " << fullpath << endl;
					found_plugin=true;
					break;
				}
				
				// dlopen failed. Make one more attempt after applying hack (eechh!)
				string err = dlerror();
				handle = FailedAttachHack(fullpath, err);
				if(handle){
					// Successfully attached plugin.
					dlclose(handle);
					cout << "Attached plugin: " << fullpath << endl;
					found_plugin=true;
					break;
				}else{
					cerr<<err<<endl;
				}
			}
		}
		
		// If we didn't find the plugin, then complain and quit
		if(!found_plugin){
			cerr<<endl;
			cerr<<"***ERROR : Couldn't find plugin \""<<plugins[j]<<"\"!                          ***" << endl;
			cerr<<"***        make sure the plugin exists in a directory specified in on of the   ***" << endl;
			cerr<<"***        following environment variables.                                    ***" << endl;
			cerr<<"***                                                                            ***" << endl;
			for(unsigned int i=0; i<envars.size(); i++){
				cerr<<"***    " << envars[i] << endl;
			}
			exit(-1);
		}
	}
}

//-----------
// FailedAttachHack
//
// Here we break our independence from GlueX code. It turns out the
// HDGEOMETRY library there uses EVIO which means the plugin we are
// attaching may require EVIO be linked in or for it to attach. One
// would *think* that specifying RTLD_LAZY would dodge any of those
// issues, but no, things just can't be easy. So, what we do here is
// the following:
//   If the dlopen call fails AND the error message contains
//   the string "evio" AND the EVIOROOT environment variable
//   is set, then we try and use the EVIOROOT variable to 
//   find the libevio.dylib and libevioxx.dylib or libevio.so
//   and libevioxx.so files and dlopen those, thus, allowing 
//   us to retry opening the plugin.
//-----------
void* FailedAttachHack(string fullpath, string err)
{
	if(err.find("evio") < err.length()){
		cout << "Failed attaching " << fullpath << endl;
		cout << "Error contains string \"evio\" so attempting to attach EVIO libraries ..." << endl;
		const char *ptr = getenv("EVIOROOT");
		if(!ptr){
			cout << "EVIOROOT is not set. Giving up!" << endl;
			return NULL;
		}
		
		string EVIOROOT(ptr);
		string base = EVIOROOT + "/lib/libevio";
		
		vector<string> suffixes;
		suffixes.push_back(".dylib");
		suffixes.push_back(".so");
		suffixes.push_back("xx.dylib");
		suffixes.push_back("xx.so");
		
		// Just try attaching all evio libraries, ignoring failures
		for(uint32_t i=0; i<suffixes.size(); i++){
			string fname = base + suffixes[i];
			dlopen(fname.c_str(), RTLD_LAZY| RTLD_GLOBAL | RTLD_NODELETE);
		}
		
		// Now, try attaching the plugin again
		void *handle = dlopen(fullpath.c_str(), RTLD_LAZY| RTLD_GLOBAL | RTLD_NODELETE);
		if(handle){
			cout << "Success!! (man, you're lucky that worked!)" << endl;
		}else{
			cout << "Dang! Oh well, we tried. This just isn't your day." << endl;
		}
		return handle;
	}

	return NULL;
}





