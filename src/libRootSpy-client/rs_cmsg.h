// $Id$
//
//    File: rs_cmsg.h
// Created: Thu Aug 28 20:00 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _rs_cmsg_
#define _rs_cmsg_

#include <sys/time.h>

#include "hinfo_t.h"

#include <vector>
#include <string>

#include <cMsg.hxx>
using namespace cmsg;

typedef struct timespec timespec_t;

#include <TMessage.h>

class rs_mainframe;

class rs_cmsg:public cMsgCallback{
	public:
		rs_cmsg(string &udl, string &name, bool connect_to_cmsg=true);
		virtual ~rs_cmsg();

		// normal requests (async)
		void PingServers(void);
		void RequestHists(string servername);
		void RequestHistogram(string servername, string hnamepath);
		void RequestHistograms(string servername, vector<string> &hnamepaths);
		void FinalHistogram(string servername, vector<string> hnamepath);
		void RequestTreeInfo(string servername);
		void RequestTree(string servername, string tree_name, string tree_path, int64_t num_entries);
		void RequestMacroList(string servername);
		void RequestMacro(string servername, string hnamepath);

		// synchronous requests
		void RequestHistsSync(string servername, timespec_t &myTimeout);
		void RequestHistogramSync(string servername, string hnamepath, timespec_t &myTimeout);
		//void FinalHistogramSync(string servername, vector<string> hnamepath);  // do we want this?
		void RequestTreeInfoSync(string servername, timespec_t &myTimeout);
		void RequestTreeSync(string servername, string tree_name, string tree_path, 
				     timespec_t &myTimeout, int64_t num_entries);
		void RequestMacroListSync(string servername, timespec_t &myTimeout);
		void RequestMacroSync(string servername, string hnamepath, timespec_t &myTimeout);

		bool   IsOnline(void)   { return is_online; }
		cMsg*  GetcMsgPtr(void) { return cMsgSys;   }
		string GetMyName(void)  { return myname;    }

		// Static method to return time in seconds with microsecond accuracy
		static double GetTime(void){
			struct timeval tval;
			struct timezone tzone;
			gettimeofday(&tval, &tzone);
			double t = (double)tval.tv_sec+(double)tval.tv_usec/1.0E6;
			if(start_time==0.0) start_time = t;
			return t - start_time;
		}

		bool hist_default_active;
		int verbose;
		static double start_time;
		string program_name;

		
	public:

		void callback(cMsgMessage *msg, void *userObject);
		void RegisterHistList(string server, cMsgMessage *msg);
		void RegisterHistogram(string server, cMsgMessage *msg);
		void RegisterHistograms(string server, cMsgMessage *msg);
		void RegisterFinalHistogram(string server, cMsgMessage *msg);
		void RegisterTreeInfo(string server, cMsgMessage *msg);
		void RegisterTreeInfoSync(string server, cMsgMessage *msg);
		void RegisterTree(string server, cMsgMessage *msg);
		void RegisterMacroList(string server, cMsgMessage *msg);
		void RegisterMacro(string server, cMsgMessage *msg);

                void BuildRequestHists(cMsgMessage &msg, string servername);
                void BuildRequestHistogram(cMsgMessage &msg, string servername, string hnamepath);
                void BuildRequestHistograms(cMsgMessage &msg, string servername, vector<string> &hnamepaths);
                void BuildRequestTreeInfo(cMsgMessage &msg, string servername);
                void BuildRequestTree(cMsgMessage &msg, string servername, string tree_name, string tree_path, int64_t num_entries);
		void BuildRequestMacroList(cMsgMessage &msg, string servername);
		void BuildRequestMacro(cMsgMessage &msg, string servername, string hnamepath);


	private:
		cMsg *cMsgSys;
		
		bool is_online;
		string myname;
		
		std::vector<void*> subscription_handles;
};


class MyTMessage : public TMessage {
public:
   MyTMessage(void *buf, Int_t len) : TMessage(buf, len) { }
};


#endif // _rs_cmsg_

