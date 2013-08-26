// $Id$
//
//    File: rs_cmsg.h
// Created: Thu Aug 28 20:00 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _rs_cmsg_
#define _rs_cmsg_

#include <vector>
#include <string>

#include <cMsg.hxx>
using namespace cmsg;

#include <TMessage.h>

class rs_mainframe;

class rs_cmsg:public cMsgCallback{
	public:
		rs_cmsg(string &udl, string &name);
		virtual ~rs_cmsg();
		
		void PingServers(void);
		void RequestHists(string servername);
		void RequestHistogram(string servername, string hnamepath);
		void FinalHistogram(string servername, vector<string> hnamepath);
		void RequestTreeInfo(string servername);

		bool IsOnline() { return is_online; }
		
	protected:

		void callback(cMsgMessage *msg, void *userObject);
		void RegisterHistList(string server, cMsgMessage *msg);
		void RegisterHistogram(string server, cMsgMessage *msg);
		void RegisterFinalHistogram(string server, cMsgMessage *msg);
		void RegisterTreeInfo(string server, cMsgMessage *msg);

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

