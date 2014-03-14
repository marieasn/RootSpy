// $Id$
//
//    File: rs_cmsg.h
// Created: Thu Aug 28 20:00 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _rs_cmsg_
#define _rs_cmsg_

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
		rs_cmsg(string &udl, string &name);
		virtual ~rs_cmsg();

		// normal requests (async)
		void PingServers(void);
		void RequestHists(string servername);
		void RequestHistogram(string servername, string hnamepath);
		void FinalHistogram(string servername, vector<string> hnamepath);
		void RequestTreeInfo(string servername);
		void RequestTree(string servername, string tree_name, string tree_path, int64_t num_entries);

		// synchronous requests
		void RequestHistsSync(string servername, timespec_t &myTimeout);
		void RequestHistogramSync(string servername, string hnamepath, timespec_t &myTimeout);
		//void FinalHistogramSync(string servername, vector<string> hnamepath);  // do we want this?
		void RequestTreeInfoSync(string servername, timespec_t &myTimeout);
		void RequestTreeSync(string servername, string tree_name, string tree_path, 
				     timespec_t &myTimeout, int64_t num_entries);

		bool IsOnline() { return is_online; }
		
	protected:

		void callback(cMsgMessage *msg, void *userObject);
		void RegisterHistList(string server, cMsgMessage *msg);
		void RegisterHistogram(string server, cMsgMessage *msg);
		void RegisterFinalHistogram(string server, cMsgMessage *msg);
		void RegisterTreeInfo(string server, cMsgMessage *msg);
		void RegisterTree(string server, cMsgMessage *msg);

                void BuildRequestHists(cMsgMessage &msg, string servername);
                void BuildRequestHistogram(cMsgMessage &msg, string servername, string hnamepath);
                void BuildRequestTreeInfo(cMsgMessage &msg, string servername);
                void BuildRequestTree(cMsgMessage &msg, string servername, string tree_name, string tree_path, int64_t num_entries);

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

