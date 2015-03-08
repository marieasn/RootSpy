// $Id$
//
//    File: rsmon_cmsg.cc
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#include <unistd.h>

#include "RootSpy.h"
#include "rsmon_cmsg.h"
#include "rs_info.h"
#include "tree_info_t.h"
#include "cMsg.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
using namespace std;



//---------------------------------
// rsmon_cmsg    (Constructor)
//---------------------------------
rsmon_cmsg::rsmon_cmsg(string myname, cMsg *cMsgSys)
{
	this->myname = myname;
	this->cMsgSys = cMsgSys;

	all_nodes[myname].program_name = "RSMonitor (this program)";

	// Subscribe to generic rootspy info requests
	all_nodes["rootspy"].subscription_handle = cMsgSys->subscribe("rootspy", "*", this, NULL);

	// Subscribe to rootspy requests specific to us
	all_nodes[myname].subscription_handle = cMsgSys->subscribe(myname, "*", this, NULL);
}

//---------------------------------
// ~rsmon_cmsg    (Destructor)
//---------------------------------
rsmon_cmsg::~rsmon_cmsg()
{
	// Unsubscribe
	map<string, nodeInfo_t>::iterator it;
	for(it=all_nodes.begin(); it!=all_nodes.end(); it++){
		void *subscription_handle = it->second.subscription_handle;
		if(subscription_handle) cMsgSys->unsubscribe(subscription_handle);
	}

}

//---------------------------------
// callback
//---------------------------------
void rsmon_cmsg::callback(cMsgMessage *msg, void *userObject)
{
	if(!msg)return;

	string subject = msg->getSubject();
	string sender = msg->getType();
	string cmd = msg->getText();
	uint64_t Nbytes = msg->getByteArrayLength();

	nodeInfo_t &ni_sender  = all_nodes[sender];
	nodeInfo_t &ni_subject = all_nodes[subject];

	if(ni_sender.subscription_handle == NULL ){
		ni_sender.subscription_handle = cMsgSys->subscribe(sender, "*", this, NULL);
	}
	
	ni_sender.cmd_types_sent_from[cmd]++;
	ni_sender.Ncmds_sent_from++;
	ni_sender.Nbytes_sent_from += Nbytes;

	ni_subject.cmd_types_sent_to[cmd]++;
	ni_subject.Ncmds_sent_to++;
	ni_subject.Nbytes_sent_to += Nbytes;
	
	
	//===========================================================
	if(cmd=="I am here"){
		if(sender != myname){
			try{
				string program = msg->getString("program");
				ni_sender.program_name = program;
			}catch(cMsgException &e){
			}
		}
	}
	//===========================================================

	delete msg;
}

//---------------------------------
// FillLines
//---------------------------------
void rsmon_cmsg::FillLines(double now, vector<string> &lines)
{
	// There are two time referneces used below. One is in
	// seconds with the zero being in unix time. This is
	// needed to compare to the "lastHeardFrom" field of the
	// RSINFO->servers. The other is the ms accurate time
	// also in seconds, but in the form of a double. It is
	// used to calculate rates more accurately.

	static double last_time = 0.0;
	double tdiff = now - last_time;
	last_time = now;

	time_t now_sec = time(NULL);
	map<string, nodeInfo_t>::iterator it;
	for(it=all_nodes.begin(); it!=all_nodes.end(); it++){
		const string &nodeName = it->first;
		nodeInfo_t &ni = it->second;

		time_t tdiff_sec = 0;
		if(nodeName!=myname && nodeName!="rootspy"){
			RS_INFO->Lock();
			tdiff_sec = now_sec - RS_INFO->servers[nodeName].lastHeardFrom;
			RS_INFO->Unlock();
			if(tdiff_sec>10) continue;
		}
		
		uint32_t Ncommandtypes_to = ni.cmd_types_sent_to.size();
		uint32_t Ncommandtypes_from = ni.cmd_types_sent_from.size();
		
		// Command rates
		double Rcmds_sent_from = (double)(ni.Ncmds_sent_from - ni.last_Ncmds_sent_from)/tdiff;
		double Rcmds_sent_to   = (double)(ni.Ncmds_sent_to   - ni.last_Ncmds_sent_to  )/tdiff;
		ni.last_Ncmds_sent_from = ni.Ncmds_sent_from;
		ni.last_Ncmds_sent_to   = ni.Ncmds_sent_to;
		char Rcmds_sent_from_str[256];
		char Rcmds_sent_to_str[256];
		sprintf(Rcmds_sent_from_str, "%5.1f", Rcmds_sent_from);
		sprintf(Rcmds_sent_to_str, "%5.1f", Rcmds_sent_to);

		// Byte rates
		double Rbytes_sent_from = (double)(ni.Nbytes_sent_from - ni.last_Nbytes_sent_from)/tdiff;
		double Rbytes_sent_to   = (double)(ni.Nbytes_sent_to   - ni.last_Nbytes_sent_to  )/tdiff;
		ni.last_Nbytes_sent_from = ni.Nbytes_sent_from;
		ni.last_Nbytes_sent_to   = ni.Nbytes_sent_to;
		char Rbytes_sent_from_str[256];
		char Rbytes_sent_to_str[256];
		int idx_from = (int)floor(log(Rbytes_sent_from)/log(10)/3.0); // 0 means <1E3  1 means <1E6  ...
		idx_from = idx_from<0 ? 0:idx_from>3 ? 3:idx_from;
		Rbytes_sent_from /= pow(10.0,(double)idx_from*3.0);
		const char *units_from = idx_from==0 ?"B":idx_from==1 ? "kB":idx_from==2 ? "MB":"GB";
		int idx_to = (int)floor(log(Rbytes_sent_to)/log(10)/3.0); // 0 means <1E3  1 means <1E6  ...
		idx_to = idx_to<0 ? 0:idx_to>3 ? 3:idx_to;
		Rbytes_sent_to /= pow(10.0,(double)idx_to*3.0);
		const char *units_to = idx_to==0 ?"B":idx_to==1 ? "kB":idx_to==2 ? "MB":"GB";
		sprintf(Rbytes_sent_from_str, "%5.1f %s/s", Rbytes_sent_from, units_from); // convert to kB
		sprintf(Rbytes_sent_to_str, "%5.1f %s/s", Rbytes_sent_to, units_to);  // convert to kB

		// Title line
		lines.push_back(nodeName + ":  " + ni.program_name);
		
		char str[256];
		stringstream ss;
		int spaces;
		int colon_col = 45;

		ss.str(std::string()); // clear ss
		sprintf(str, "Num. unique commands sent to:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << Ncommandtypes_to;
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Num. unique commands sent from:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << Ncommandtypes_from;
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Total commands sent to:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << ni.Ncmds_sent_to << "  (" << Rcmds_sent_to_str << "Hz)";
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Total commands sent from:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << ni.Ncmds_sent_from << "  (" << Rcmds_sent_from_str << "Hz)";
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Total bytes sent to:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << ni.Nbytes_sent_to << "  (" << Rbytes_sent_to_str << ")";
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Total bytes sent from:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << ni.Nbytes_sent_from << "  (" << Rbytes_sent_from_str << ")";
		lines.push_back(ss.str());

		ss.str(std::string()); // clear ss
		sprintf(str, "Seconds since last heard from:");
		spaces = colon_col - string(str).length();
		if(spaces<1) spaces = 1;
		ss << string(spaces, ' ');
		ss << str << tdiff_sec << "s ago";
		lines.push_back(ss.str());
		
		lines.push_back("");
	}
}


