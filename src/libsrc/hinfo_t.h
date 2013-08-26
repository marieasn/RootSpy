// $Id$
//
//    File: hinfo_t.h
// Created: Thursday December 10, 2009
// Creator: 
//

#ifndef _hinfo_t_
#define _hinfo_t_

#include <vector>
#include <list>
#include <map>
#include <TDirectory.h>
#include <TH1.h>
#include <string>

using namespace std;

#include <pthread.h>

class server_info_t;
class TH1;

/// Due to the asynchronous nature of this program, it is difficult to
/// use pointers to keep track of objects in the traditional C++ way.
/// Most information is kept in containers and those may be modified
/// at any time causing all of their elements to be relocated in memory
/// making it dangerous to keep pointers to individual elements outside
/// of mutex locks.
///
/// The solution here is to use the minimal amount of info to uniquely
/// identify a histogram+server bundled into an object that can be passed
/// around in place of a pointer. Unfortunately, one will still need to
/// lock a mutex, find a pointer to the hinfo_t object using the hinfoid_t
/// and then release the mutex after they are done with the hinfo_t pointer.
///
/// To accomodate simpler code for finding the right hinfo_t structure,
/// the hinfo_t class is made to derive from hinfoid_t.



/// in hinfo_t, the information for each of the hists is stored,
/// ie: name, path, type, bin limit, and some flags;

class hid_t{
	public:
	
		hid_t(string server, string hnamepath){
			this->serverName = server;
			this->hnamepath = hnamepath;
		}
		
		hid_t(){
			this->serverName = "";
			this->hnamepath = "";
		}
	
		string hnamepath;			// path + "/" + name
		string serverName;
		
		bool operator==(const hid_t &h) const {
			return h.hnamepath==hnamepath && h.serverName==serverName;
		}
		bool operator!=(const hid_t &h) const {
			return !((*this)==h);
		}
		bool operator<(const hid_t &h) const {
			if(h.serverName==serverName)return h.hnamepath<hnamepath;
			return h.serverName<serverName;
		}
};


/// in hid_t, the information for each of the hists is stored,
/// ie: name, path, type, bin limit, and some flags;


class hinfo_t:public hid_t{
	public:
		
		hinfo_t(string server, string hnamepath):hid_t(server, hnamepath){
			received = 0;
			hist = NULL;
			active = true;
			hasBeenDisplayed = false;
			isDisplayed = false;
		}
		hinfo_t():hid_t("",""){
			received = 0;
			hist = NULL;
			active = false;
			hasBeenDisplayed = false;
			isDisplayed = false;
		}
		
		bool operator== (const hinfo_t& hi);

		time_t received;
		TH1* hist;
		
		bool active;					// sees if the histogram is usable
		bool hasBeenDisplayed;		// makes sure that each hist is not displayed more than once
				
		bool getDisplayed() {return isDisplayed;}
		void setDisplayed(bool newDisplay) {isDisplayed = newDisplay;}
		
		private:
			bool isDisplayed;				// true only when the "loop over all servers" button is down and it is drawn

};

#endif // _hinfo_t_

		
