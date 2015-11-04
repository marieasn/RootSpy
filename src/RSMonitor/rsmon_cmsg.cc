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
#include "rs_cmsg.h"
#include "tree_info_t.h"
#include "cMsg.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
using namespace std;


extern string ROOTSPY_UDL;
extern string CMSG_NAME;
extern double START_TIME;

bool sizeSort (const pair<string,double> &a, const pair<string,double> &b) { return (b.second<a.second); }


//---------------------------------
// rsmon_cmsg    (Constructor)
//---------------------------------
rsmon_cmsg::rsmon_cmsg(string myname, cMsg *cMsgSys)
{

	// If the cMsgSys pointer is NULL, then make our own connection
	if(cMsgSys==NULL){
		string myDescr = "Access ROOT objects in a running program";
		cMsgSetDebugLevel(CMSG_DEBUG_WARN); // print most messages
		cMsgSys = new cMsg(ROOTSPY_UDL, CMSG_NAME, myDescr);   	// the cMsg system object, where
		try {                         	           	//  all args are of type string
			cMsgSys->connect();
			cMsgSys->start();
		} catch (cMsgException e) {
			cout<<endl<<endl<<endl<<endl<<"_______________  ROOTSPY unable to connect to cMsg system! __________________"<<endl;
			cout << e.toString() << endl; 
			cout<<endl<<endl<<endl<<endl;

			// we need to connect to cMsg to run
			exit(0);          
		}
	}

	this->myname = myname;
	this->cMsgSys = cMsgSys;
	this->focus_node = "";
	this->include_rootspy_in_stats = true;
	this->respond_to_pings=true;

	all_nodes[myname].program_name = "RSMonitor (this program)";

	// Subscribe to generic rootspy info requests
	all_nodes["rootspy"].subscription_handle = cMsgSys->subscribe("rootspy", "*", this, NULL);
	all_nodes["rootspy"].program_name = "(all processes)";

	// Subscribe to rootspy requests specific to us
//	all_nodes[myname].subscription_handle = cMsgSys->subscribe(myname, "*", this, NULL);
	all_nodes[myname].subscription_handle = cMsgSys->subscribe("rs_*", "*", this, NULL);
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
//		ni_sender.subscription_handle = cMsgSys->subscribe(sender, "*", this, NULL);
	}
	
	ni_sender.cmd_types_sent_from[cmd]++;
	ni_sender.Ncmds_sent_from++;
	ni_sender.Nbytes_sent_from += Nbytes;
	ni_sender.lastHeardFrom = rs_cmsg::GetTime() - START_TIME;

	ni_subject.cmd_types_sent_to[cmd]++;
	ni_subject.Ncmds_sent_to++;
	ni_subject.Nbytes_sent_to += Nbytes;
	
	// User may specify whether things sent to "rootspy" are included
	// in stats for individual nodes.
	if(include_rootspy_in_stats && subject=="rootspy"){
		map<string, nodeInfo_t>::iterator it;
		for(it=all_nodes.begin(); it!=all_nodes.end(); it++){
			const string &nodeName = it->first;
			nodeInfo_t &ni = it->second;
			if(nodeName == "rootspy") continue; // already handled above
			
			ni.cmd_types_sent_to[cmd]++;
			ni.Ncmds_sent_to++;
			ni.Nbytes_sent_to += Nbytes;
		}
	}
	
	//===========================================================
	if(cmd=="I am here"){
		if(sender != myname){
			try{
				string program = msg->getString("program");
				ni_sender.program_name = program;
			}catch(cMsgException &e){
			}
		}
	}else if(cmd=="who's there?"){
		if(respond_to_pings){
			cMsgMessage *response = new cMsgMessage();
			response->setSubject(sender);
			response->setType(myname);
			response->setText("I am here");
			response->add("program", "RSMonitor");
			cMsgSys->send(response);
			delete response;
		}
	}else if(cmd=="histogram"){
		string hnamepath = msg->getString("hnamepath");
		double sizekB = (double)msg->getByteArrayLength()/1000.0;
		message_sizes[string(" HIST - ") + hnamepath] = sizekB;
	}else if(cmd=="tree"){
    	string name = msg->getString("tree_name");
    	string path = msg->getString("tree_path");
		string hnamepath = path + "/" + name;
		double sizekB = (double)msg->getByteArrayLength()/1000.0;
		message_sizes[string(" TREE - ") + hnamepath] = sizekB;
	}else if(cmd=="tree"){
    	string name = msg->getString("macro_name");
    	string path = msg->getString("macro_path");
		string hnamepath = path + "/" + name;
		double sizekB = (double)msg->getByteArrayLength()/1000.0;
		message_sizes[string("MACRO - ") + hnamepath] = sizekB;
	}
	//===========================================================

	delete msg;
}

//---------------------------------
// FillLines
//---------------------------------
void rsmon_cmsg::FillLines(double now, vector<string> &lines)
{

	static double last_time = 0.0;
	double tdiff = now - last_time;
	last_time = now;

	// If user has specified a "focus" node, then only print lines for that
	// node along with details on the commands sent to/from. If the specified
	// focus node does not exist, then print that as a message.
	bool has_focus_node = (focus_node!="");
	if(has_focus_node){
		map<string, nodeInfo_t>::iterator it = all_nodes.find(focus_node);
		if(it == all_nodes.end()){
			lines.push_back("");
			lines.push_back("The node \"" + focus_node + "\" was specified");
			lines.push_back("as the focus node (i.e. have detailed info printed)");
			lines.push_back("but it is not in the list of active nodes.");
			return;
		}
	}

	map<string, nodeInfo_t>::iterator it;
	for(it=all_nodes.begin(); it!=all_nodes.end(); it++){
		const string &nodeName = it->first;
		nodeInfo_t &ni = it->second;
		
		if(has_focus_node && (nodeName!=focus_node)) continue;

		time_t tdiff_sec = 0;
		if(nodeName!=myname && nodeName!="rootspy"){
			tdiff_sec = now - ni.lastHeardFrom;
			if(tdiff_sec>10) continue;
		}
		
		uint32_t Ncommandtypes_to = ni.cmd_types_sent_to.size();
		uint32_t Ncommandtypes_from = ni.cmd_types_sent_from.size();
		
		// Individual Command rates
		map<string, double> Rcmd_types_sent_from;
		map<string, uint32_t>::iterator it_types;
		for(it_types=ni.cmd_types_sent_from.begin(); it_types!=ni.cmd_types_sent_from.end(); it_types++){
			const string &cmd = it_types->first;
			uint32_t calls = it_types->second;
			uint32_t last_calls = ni.last_cmd_types_sent_from[cmd];
			ni.last_cmd_types_sent_from[cmd] = calls;
			Rcmd_types_sent_from[cmd] = (double)(calls - last_calls)/tdiff;
		}
		map<string, double> Rcmd_types_sent_to;
		for(it_types=ni.cmd_types_sent_to.begin(); it_types!=ni.cmd_types_sent_to.end(); it_types++){
			const string &cmd = it_types->first;
			uint32_t calls = it_types->second;
			uint32_t last_calls = ni.last_cmd_types_sent_to[cmd];
			ni.last_cmd_types_sent_to[cmd] = calls;
			Rcmd_types_sent_to[cmd] = (double)(calls - last_calls)/tdiff;
		}

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

		// Title line (write in bold)
		stringstream ss;
		ss << "BOLD" << nodeName << ":  " << ni.program_name;
		lines.push_back(ss.str());
		
		char str[256];
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
		
		// Only print the following a focus node was specified
		if(has_focus_node){
			lines.push_back("Commands sent from:");
			map<string, uint32_t>::iterator it_types;
			for(it_types=ni.cmd_types_sent_from.begin(); it_types!=ni.cmd_types_sent_from.end(); it_types++){
				const string &cmd = it_types->first;
				double rate = Rcmd_types_sent_from[cmd];
				
				ss.str(std::string()); // clear ss
				sprintf(str, "%6d (%5.2fHz)",it_types->second ,rate);
				spaces = colon_col - cmd.length();
				if(spaces<1) spaces = 1;
				ss << string(spaces, ' ');
				ss << cmd << ": " << str;
				lines.push_back(ss.str());
			}
			
			lines.push_back("");
			lines.push_back("Commands sent to:");
			for(it_types=ni.cmd_types_sent_to.begin(); it_types!=ni.cmd_types_sent_to.end(); it_types++){
				const string &cmd = it_types->first;
				double rate = Rcmd_types_sent_to[cmd];
				
				ss.str(std::string()); // clear ss
				sprintf(str, "%6d (%5.2fHz)",it_types->second ,rate);
				spaces = colon_col - cmd.length();
				if(spaces<1) spaces = 1;
				ss << string(spaces, ' ');
				ss << cmd << ": " << str;
				lines.push_back(ss.str());
			}
		}		
	}
}

//---------------------------------
// FillLinesMessageSizes
//---------------------------------
void rsmon_cmsg::FillLinesMessageSizes(double now, vector<string> &lines)
{
	// Copy the map into a vector so we can sort it by size
	// At the same time, record some cumulative info.
	vector<pair<string, double> > sorted;
	double tot_sizekB = 0.0;
	map<string,double> path_tot; // key=path  val=total size in kB
	map<string,uint32_t> path_Nobjs; // key=path  val=num. of hnamepaths in path
	map<string, double>::iterator it;
	for(it=message_sizes.begin(); it!=message_sizes.end(); it++){
		string hnamepath(it->first);
		double sizekB = it->second;
		sorted.push_back(pair<string, double>(hnamepath, sizekB));
		
		tot_sizekB += sizekB;
		size_t pos1 = hnamepath.find("/");
		if(pos1 != string::npos){
			size_t pos2 = hnamepath.find("/",pos1+1);
			if(pos2 != string::npos){
				string path = hnamepath.substr(pos1, pos2-pos1);
				path_tot[path] += sizekB;
				path_Nobjs[path]++;
			}
		}
	}
	
	// Copy the path_tot map into a vector so we can sort it
	vector<pair<string, double> > path_tot_sorted; // key=path  val=total size in kB
	for(it=path_tot.begin(); it!=path_tot.end(); it++){
		string path(it->first);
		double sizekB = it->second;
		path_tot_sorted.push_back(pair<string, double>(path, sizekB));
	}

	// Sort by size
	sort(sorted.begin(), sorted.end(), sizeSort);
	sort(path_tot_sorted.begin(), path_tot_sorted.end(), sizeSort);

	char str[256];
	sprintf(str, "    Total size : %6.1f MB", tot_sizekB/1000.0);
	lines.push_back(str);
	lines.push_back("");
	lines.push_back("    Biggest paths (limit 4):");
	for(uint32_t i=0; i<path_tot_sorted.size(); i++){
		if(i>=4) break;
		string path = path_tot_sorted[i].first;
		double sizekB = path_tot_sorted[i].second;
		sprintf(str, "       %6.1f kB : %4d items : %s", sizekB, path_Nobjs[path], path.c_str());
		lines.push_back(str);
	}
	lines.push_back("");

	// Add lines
	for(uint32_t i=0; i<sorted.size(); i++){
		string label = sorted[i].first;
		double sizekB = sorted[i].second;
		sprintf(str, " %6.1f kB : %s", sizekB, label.c_str());
		lines.push_back(str);
	}
}

//---------------------------------
// PingServers
//---------------------------------
void rsmon_cmsg::PingServers(void)
{
	cMsgMessage whosThere;
	whosThere.setSubject("rootspy");
	whosThere.setType(myname);
	whosThere.setText("who's there?");
	cMsgSys->send(&whosThere);
}
