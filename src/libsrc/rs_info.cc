// $Id$
//
//    File: rs_info.cc
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include <iostream>
#include <iomanip>
#include <algorithm>
using namespace std;


#include "rs_info.h"

extern rs_info *RS_INFO;

bool sortPredicate(const hid_t& hid1, const hid_t& hid2){
	if(hid1.serverName != hid2.serverName){
		return hid1.serverName < hid2.serverName;
	} else{
		return hid1.hnamepath < hid2.hnamepath;
	}
}

//---------------------------------
// rs_info    (Constructor)
//---------------------------------
rs_info::rs_info()
{
	pthread_mutex_init(&mutex, NULL);

	current.serverName = "N/A";
	current.hnamepath = "-------------------------------------------------------";
	
	sum_dir = new TDirectory("rootspy", "RootSpy local");
	update = false;
}

//---------------------------------
// ~rs_info    (Destructor)
//---------------------------------
rs_info::~rs_info()
{
	if(sum_dir != NULL) {delete sum_dir;}
}

//---------------------------------
// GetActiveHIDs
//---------------------------------
void rs_info::GetActiveHIDs(vector<hid_t> &active_hids)
{
	// This should be called while RS_INFO->Lock is active to ensure
	// RS_INFO isn't changing. We don't lock it here since the methods
	// calling us also need to access RS_INFO and we need to make sure
	// it is not changing through the whole call chain.

	// Make list of all active hids
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.begin();
	for(; hdef_iter!=RS_INFO->histdefs.end(); hdef_iter++){
		map<string, bool>::iterator server_iter = hdef_iter->second.servers.begin();
		for(; server_iter!=hdef_iter->second.servers.end(); server_iter++){
			
			const string &server = server_iter->first;
			const string &hnamepath = hdef_iter->first;
			
			// Find server_info_t object to see if server itself is active
			map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
			if(server_info_iter!=RS_INFO->servers.end()){
	
				bool server_active = server_info_iter->second.active;
				bool hdef_active = hdef_iter->second.active;
				bool hid_active = server_iter->second;
				bool active;
				
				// The histogram is "active" based on the flags and the viewStyle
				if(RS_INFO->viewStyle==rs_info::kViewByObject){
					active = server_active && hid_active;
				}else{
					active = hdef_active && hid_active;
				}
				
				if(active){
					hid_t hid(server, hnamepath);
					active_hids.push_back(hid);
				}
			}
		}
	}
	
}

//---------------------------------
// FindNextActive
//---------------------------------
hid_t rs_info::FindNextActive(hid_t &current)
{
	// Get list of all active hids
	unsigned int current_index=0;
	vector<hid_t> active_hids;
	GetActiveHIDs(active_hids);
	if(active_hids.size()<1){return current;}
	
	// Since we have at least one HID, it is time to sort them. //
	std::sort(active_hids.begin(), active_hids.end(), sortPredicate);
	
	// Find index of "current" hid
	for(unsigned int i=0; i<active_hids.size(); i++){
		if(active_hids[i]==current){
			current_index = i;
			break;
		}
	}

	// If viewing by server, just set to the next object. If viewing by object, then
	// we need to loop until we find the next hnamepath
	if(RS_INFO->viewStyle==rs_info::kViewByObject){
		for(unsigned int i=0; i<active_hids.size(); i++){
			hid_t &hid = active_hids[(i+current_index+1)%active_hids.size()];
			if(hid.hnamepath!=current.hnamepath){
				return hid;
			}
		}
	}else{
		return active_hids[(current_index+1)%active_hids.size()];
	}
}

//---------------------------------
// FindPreviousActive
//---------------------------------
hid_t rs_info::FindPreviousActive(hid_t &current)
{
	// Get list of all active hids
	unsigned int current_index=0;
	vector<hid_t> active_hids;
	GetActiveHIDs(active_hids);
	if(active_hids.size()<1){return current;}
	// Since we have at least one HID, it is time to sort them. //
	std::sort(active_hids.begin(), active_hids.end(), sortPredicate);
	
	// Find index of "current" hid
	for(unsigned int i=0; i<active_hids.size(); i++){
		if(active_hids[i]==current){
			current_index = i;
			break;
		}
	}

	// If viewing by server, just set to the previous object. If viewing by object, then
	// we need to loop until we find the previous hnamepath
	if(RS_INFO->viewStyle==rs_info::kViewByObject){
		for(unsigned int i=active_hids.size(); i>0; i--){
			hid_t &hid = active_hids[(i+current_index-1)%active_hids.size()];
			if(hid.hnamepath!=current.hnamepath){
				return hid;
			}
		}
	}else{
		return active_hids[(active_hids.size()+current_index-1)%active_hids.size()];
	}
}
