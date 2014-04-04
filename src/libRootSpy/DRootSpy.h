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

#include <vector>

#include <TDirectory.h>
#include <cMsg.hxx>
#include <pthread.h>
using namespace cmsg;


class DRootSpy:public cMsgCallback{
 public:
    DRootSpy(string udl="<default>");
    virtual ~DRootSpy();
    
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
    TDirectory *hist_dir; // save value of gDirectory used when forming response to "list hist" request
    string myname;
    std::vector<void*> subscription_handles;
    vector<string> *finalhists;
    pthread_t mythread;
    string finalsender;
    
    //methods
    void traverseTree(TObjArray *branch_list, vector<string> &treeinfo);
    void listHists(cMsgMessage &response);
    void getHist(cMsgMessage &response, string &hnamepath);
    void getTree(cMsgMessage &response, string &name, string &path, int64_t nentries);
    void treeInfo(string sender);
    void treeInfoSync(cMsgMessage &response, string sender);
};

#endif // _DRootSpy_
