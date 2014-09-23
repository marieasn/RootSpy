// $Id$
//
//    File: rs_info.cc
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include "rs_info.h"
#include "rs_cmsg.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
using namespace std;


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
// RequestHistograms
//---------------------------------
int rs_info::RequestHistograms(string hnamepath, bool lock_mutex)
{
	/// Request the specified histogram or macro from all servers
	/// that provide it. If the lock_mutex value is set to false,
	/// then the RS_INFO lock will not be locked during the operation.
	/// In that case it is assumed that the calling routine has already
	/// locked it.

	int NrequestsSent = 0;

	if(lock_mutex) Lock();

	map<string,hdef_t>::iterator it = histdefs.find(hnamepath);
	if(it != histdefs.end()){
		
		// Loop over servers, requesting this hist from each one (that is active)
		map<string, bool> &servers = it->second.servers;
		map<string, bool>::iterator server_iter = servers.begin();
		for(; server_iter!=servers.end(); server_iter++){
			//if(server_iter->second){
				NrequestsSent++;
				if(it->second.type == hdef_t::macro)
					RS_CMSG->RequestMacro(server_iter->first, hnamepath);
				else
					RS_CMSG->RequestHistogram(server_iter->first, hnamepath);
			//}
		}
	}
	
	if(lock_mutex) Unlock();
	
	return NrequestsSent;
}

//---------------------------------
// GetSumHist
//---------------------------------
TH1* rs_info::GetSumHist(string &hnamepath, hdef_t::histdimension_t *type, double *sum_hist_modified, string *servers_str)
{
	/// Look through list of currently defined histograms/macros and return info on the
	/// summed histogram. If the histogram is found, a pointer to it is returned.
	/// If the type, sum_hist_modified, and servers_str pointers are given, then the values
	/// are copied into there from the the hdef_t object. If the hnamepath is not found, then
	/// NULL is returned and the type is set to hdef_t::noneD.
	///
	/// The value copied into servers_str (if not NULL) is a representation of the servers
	/// contributing to the sum. If a single server is all that is contributing, then that
	/// server is copied in. If more than one, then a string starting with "(N svrs)"
	/// is copied followed by the names or partial names of the first few servers such that
	/// the string does not exceed 25 characters. This is primarily here for the RootSpy
	/// GUI for it to put into the "server" label when displaying a hist.

	Lock();

	TH1 *sum_hist = NULL;
	if(type) *type = hdef_t::noneD;
	if(sum_hist_modified) *sum_hist_modified = 0.0;
	
	map<string,hdef_t>::iterator it = histdefs.find(hnamepath);
	if(it != histdefs.end()){
		hdef_t &hdef = it->second;

		sum_hist = hdef.sum_hist;
		if(type) *type = hdef.type;
		if(sum_hist_modified) *sum_hist_modified = hdef.sum_hist_modified;
		if(servers_str){
			if(hdef.hists.size() == 0){
				*servers_str = "<none>";
			}else if(hdef.hists.size() == 1){
				*servers_str = (hdef.hists.begin())->first;
			}else{
				stringstream ss;
				ss << "(" << hdef.hists.size() << " servers)\n";
				map<string, hinfo_t>::iterator it_srv = hdef.hists.begin();
				int Nlines=1;
				for(; it_srv!=hdef.hists.end(); it_srv++){
					ss << it_srv->first << "\n";
					if(++Nlines >=6){
						ss << "...";
					}
				}
				*servers_str = ss.str();
			}
		}
	}

	Unlock();
	
	return sum_hist;
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

	return hid_t("none", "none");  // avoid compiler warning
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
	
	return hid_t("none", "none");  // avoid compiler warning
}


//---------------------------------
// Reset
//---------------------------------
void rs_info::Reset()
{
	Lock();

    // delete histograms
    for(map<hid_t,hinfo_t>::iterator hit = hinfos.begin();
	hit != hinfos.end(); hit++) {
	delete hit->second.hist;
    }

    // delete trees
    for(map<string,server_info_t>::iterator servit = servers.begin();
	servit != servers.end(); servit++) {

	for(vector<tree_info_t>::iterator treeit = servit->second.trees.begin();
	    treeit != servit->second.trees.end(); treeit++) {
	    delete treeit->tree;
	}

    }

    // delete saved histograms
    // not entirely sure from the documentation if this is the correct 
    // function - might need Delete()?
    sum_dir->Clear();  
    
    // clear up saved info
    servers.clear();
    histdefs.clear();
    hinfos.clear();

    // zero out the current histogram def
    current = hid_t();
    //current.hnamepath = "";
	
	Unlock();
}
