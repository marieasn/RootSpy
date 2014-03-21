// $Id$
//
//    File: server_info_t.h
// Created: Thursday December 10, 2009
// Creator: 
//

#ifndef _server_info_t_
#define _server_info_t_

#include <vector>
#include <list>
#include <map>
#include <TDirectory.h>
#include <TH1.h>

#include <pthread.h>

#include "RootSpy.h"
#include "hinfo_t.h"
#include "tree_info_t.h"

class TH1;

/// in server_info_t, the information for each of the server is stored,
/// ie: name, time, the hist info, and the current activity

class server_info_t{

	public:
		server_info_t(string serverName){
			this->serverName = serverName; 
			lastHeardFrom = time(NULL);
			dir = new TDirectory(serverName.c_str(),serverName.c_str());
			active = true;
		}
		server_info_t():dir(NULL){}
		~server_info_t(){
			if(dir){
				// We do not actually delete the TDirectory here because temporary server_info_t
				// objects created on the stack will copy the pointer, but then have their
				// destructors called as they go out of scope deleting the TDirectory leaving
				// an invalid pointer in the RS_INFO->servers object. See rs_cmsg::callback()
				// for where this takes place.
				//delete dir;
			}
		}

		string serverName;				// the name of the server
		time_t lastHeardFrom;			// the last time a message was received from server
		TDirectory *dir;				// directory to hold histograms from this server

		vector<string> hnamepaths;		// hnamepaths of histos available on this server
		vector<tree_info_t> trees;		// trees available on this server.
		bool active;					// include this sever's histos in display?

};

#endif // _server_info_t_
