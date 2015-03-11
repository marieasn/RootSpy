// $Id$
//
//    File: rsmon_cmsg.h
// Created: Fri Mar  6 11:28:22 EST 2015
// Creator: davidl (on Linux gluon104.jlab.org 2.6.32-358.23.2.el6.x86_64)
//

#ifndef _rsmon_cmsg_
#define _rsmon_cmsg_

#include <sys/time.h>

#include <set>
#include <vector>
#include <string>
using namespace std;

#include <cMsg.hxx>
using namespace cmsg;

typedef struct timespec timespec_t;

class rsmon_cmsg:public cMsgCallback{
	public:
		rsmon_cmsg(string myname, cMsg *cMsgSys);
		virtual ~rsmon_cmsg();
		
		class nodeInfo_t{
			public:
				nodeInfo_t(void* subscription_handle=NULL){
					this->subscription_handle = subscription_handle;
					program_name = "<unknown>";
					Ncmds_sent_from = 0;
					Ncmds_sent_to = 0;
					last_Ncmds_sent_from = 0;
					last_Ncmds_sent_to = 0;
					Nbytes_sent_from = 0;
					Nbytes_sent_to = 0;
					last_Nbytes_sent_from = 0;
					last_Nbytes_sent_to = 0;
				}
			
				void* subscription_handle;
				string program_name;
				double lastHeardFrom;

				map<string, uint32_t> cmd_types_sent_from;
				map<string, uint32_t> cmd_types_sent_to;

				map<string, uint32_t> last_cmd_types_sent_from;
				map<string, uint32_t> last_cmd_types_sent_to;

				uint32_t Ncmds_sent_from;
				uint32_t Ncmds_sent_to;
				
				uint32_t last_Ncmds_sent_from;
				uint32_t last_Ncmds_sent_to;
				
				uint64_t Nbytes_sent_from;
				uint64_t Nbytes_sent_to;
				
				uint64_t last_Nbytes_sent_from;
				uint64_t last_Nbytes_sent_to;
		};
		
	public:

		string focus_node;
		bool include_rootspy_in_stats;
		bool respond_to_pings;

		void callback(cMsgMessage *msg, void *userObject);		
		void FillLines(double now, vector<string> &lines);
		void PingServers(void);

	private:
		cMsg *cMsgSys;
		
		string myname;
		
		map<string, nodeInfo_t> all_nodes;

};


#endif // _rsmon_cmsg_

