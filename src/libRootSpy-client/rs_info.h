// $Id$
//
//    File: rs_info.h
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#ifndef _rs_info_
#define _rs_info_

#include "hinfo_t.h"
#include "hdef_t.h"
#include "server_info_t.h"
#include "final_hist_t.h"

#include <vector>
#include <list>
#include <map>
#include <deque>
#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>

#include <pthread.h>

//#include <cMsg.hxx>
//using namespace cmsg;

class TH1;

/// The rs_info class holds information obtained from the server(s). It serves as a storing
/// area for information received in the cMsg callback. It is done this way so that the
/// ROOT GUI thread can access it inside of rs_mainframe::DoTimer() at it's convenience.
/// ROOT is generally not happy about 2 threads accessing it's global memory.

class rs_info{
	public:
		rs_info();
		virtual ~rs_info();

		enum viewStyle_t{
			kViewByObject,
			kViewByServer
		};

// ROOT's cint doesn't like pthread_mutex_t so we mask it off here.
// The only thing it needs to know about is the viewStyle_t enum
#ifndef __CINT__

		void Lock(void){pthread_mutex_lock(&mutex);}
		void Unlock(void){pthread_mutex_unlock(&mutex);}
		
		map<string,server_info_t> servers;	    // key=server	    val=server info.
		map<string,hdef_t> histdefs;			// key=hnamepath    val=histogram definition
		map<hid_t,hinfo_t> hinfos;				// key=server/hist	val=histogram info
		map<string,tree_id_t> treedefs;    // key=tnamepath val=tree definition
		
		TDirectory *sum_dir;	// holds sum histograms
		
		hid_t current;
		//vector<hid_t> current_hists;            // for multi-histogram display
		viewStyle_t viewStyle;
		bool update;				// flag indicating rs_mainframe::DoUpdate should be called on next DoTimer call

		void  GetActiveHIDs(vector<hid_t> &active_hids);
		int   RequestHistograms(string hnamepath, bool lock_mutex=true); // returns number of servers a request was sent to
		int   RequestTrees(string tnamepath, int64_t num_entries, bool lock_mutex);
		TH1*  GetSumHist(string &hnamepath, hdef_t::histdimension_t *type=NULL, double *sum_hist_modified=NULL, string *servers_str=NULL);
		hid_t FindNextActive(hid_t &current);
		hid_t FindPreviousActive(hid_t &current);
		void  LoadRootFile(string fname);
		void  AddRootObjectsToList(TDirectory *dir);
		void  TraverseTree(TObjArray *branch_list, vector<string>  &treeinfo);
		void  LoadMacro(string name, string path, string macro_data);

		void Reset();

		//deque<cMsgMessage> final_messages;
		deque<final_hist_t> final_hists;

	protected:
	
		TFile *rootfile; // used if reading from file rather than from cMsg system
	
	private:
		pthread_mutex_t mutex;

		//#else
		//Class//Def(rs_info,1)
#endif //__CINT__
};

#endif // _rs_info_

