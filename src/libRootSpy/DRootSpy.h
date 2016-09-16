// $Id$
//
//    File: DRootSpy.h
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//
 	
#if HAVE_CONFIG_H
#include <rootspy_config.h>
#endif

#ifndef _DRootSpy_
#define _DRootSpy_

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/time.h>


#include <vector>
#include <string>

#include "macro_info_t.h"

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <cMsg.hxx>
#include <pthread.h>
using namespace cmsg;

extern pthread_rwlock_t *gROOTSPY_RW_LOCK;
extern string gROOTSPY_PROGRAM_NAME;


class DRootSpy:public cMsgCallback{
 public:
    DRootSpy(string udl);
    DRootSpy(pthread_rwlock_t *rw_lock=NULL, string udl="<default>");
    void Initialize(pthread_rwlock_t *rw_lock, string udl);
    virtual ~DRootSpy();

	 void ConnectToCMSG(void);
	 void* WatchConnection(void);
	 void* DebugSampler(void);
    
    class hinfo_t{
    public:
	string name;
	string title;
	string type;
	string path;
    };
    class tree_info_t {
    public:
	string name;
	string title;
	string path;
	vector<string> branch_info;
    };
    void ReturnFinals(void);

    // macro functions
    bool RegisterMacro(string name, string path, string macro_data);
    bool AddMacroHistogram(string name, string path, TH1 *the_hist);
    bool AddMacroHistogram(string name, string path, vector<TH1*> the_hists);
    bool SetMacroVersion(string name, string path, int version_number);

	static double GetTime(void){
		struct timeval tval;
		struct timezone tzone;
		gettimeofday(&tval, &tzone);
		double t = (double)tval.tv_sec+(double)tval.tv_usec/1.0E6;
		if(start_time==0.0) start_time = t;
		return t - start_time;
	}
	static double start_time;
	
	 map<string, map<string, double> > last_sent; // outer key=server, inner key=hnamepath

	 int VERBOSE;
	 bool DEBUG;
	 
	 bool in_callback;
	 bool in_list_hists;
	 bool in_list_macros;
	 bool in_get_hist;
	 bool in_get_hists;
	 bool in_get_tree;
	 bool in_get_tree_info;
	 bool in_get_macro;
	 
	 enum{
	 
	 	kNSAMPLES,
	 	kREADLOCKED,
		kWRITELOCKED,

		kINCALLBACK,
		kIN_LIST_HISTS,
		kIN_LIST_MACROS,
		kIN_GET_HIST,
		kIN_GET_HISTS,
		kIN_GET_TREE,
		kIN_GET_TREE_INFO,
		kIN_GET_MACRO,
		
		kNCOUNTERS
	 }RSBinDefs_t;
	 
	 TFile *debug_file;
	 TH1I *hcounts;
	 TH1D *hfractions;

 protected:
    
    void callback(cMsgMessage *msg, void *userObject);
    void addRootObjectsToList(TDirectory *dir, vector<hinfo_t> &hinfos);
    void addTreeObjectsToList(TDirectory *dir, vector<tree_info_t> &tree_infos);
    void findTreeObjForMsg(TDirectory *dir, string sender);
    void findTreeNamesForMsg(TDirectory *dir, vector<string> &tree_names,
			     vector<string> &tree_titles, vector<string> &tree_paths);     

 private:
    //class variables
    cMsg *cMsgSys;
    bool own_gROOTSPY_RW_LOCK;
    TDirectory *hist_dir; // save value of gDirectory used when forming response to "list hist" request
    string myname;
	 string myudl;
    std::vector<void*> subscription_handles;
    vector<string> *finalhists;
    pthread_t mythread;
    pthread_t mywatcherthread;
    pthread_t mydebugthread;
    string finalsender;
	 bool done;

    map<string,macro_info_t> macros;

    //methods
    void traverseTree(TObjArray *branch_list, vector<string> &treeinfo);
    void listHists(cMsgMessage &response);
    void getHist(cMsgMessage &response, string &hnamepath, bool send_message=true);
//	 void getHistUDP(cMsgMessage &response, string hnamepath, uint32_t addr32, uint16_t port);
	 void getHistUDP(void *response, string hnamepath, uint32_t addr32, uint16_t port);
    void getHists(cMsgMessage &response, vector<string> &hnamepaths);
    void getTree(cMsgMessage &response, string &name, string &path, int64_t nentries);
    void treeInfo(string sender);
    void treeInfoSync(cMsgMessage &response, string sender);
    void listMacros(cMsgMessage &response);
    void getMacro(cMsgMessage &response, string &hnamepath);

};

extern DRootSpy *gROOTSPY;


#endif // _DRootSpy_

