// $Id$
//
//    File: hdef_t.h
// Created: Thursday December 10, 2009
// Creator: 
//

#ifndef _hdef_t_
#define _hdef_t_

#include <vector>
#include <list>
#include <map>
#include <TDirectory.h>
#include <TH1.h>
#include <string>

using namespace std;

#include <pthread.h>

class server_info_t;
class hinfo_t;
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

class hdisplay_info_t {
public:
    bool use_logx;
    bool use_logy;
    
    int overlay_scale_mode;  // 0=none, 1=full range, 2=bin range, 3=percent range
    double scale_range_low, scale_range_hi;


    hdisplay_info_t() {
	use_logx = false;
	use_logy = false;

	overlay_scale_mode = 1;
	scale_range_low = 0;
	scale_range_hi = 100;
    }
};


class hdef_t{
	public:
	enum histdimension_t {noneD, oneD, twoD, threeD,macro};		// 2D, 3D, etc.
		
		hdef_t(string name, string path){
			this->name = name;
			this->path = path;
			this->hnamepath = path + "/" + name;
			this->sum_hist=NULL;
			sum_hist_present = false;

			// Split path at "/" to get directory structures
			size_t cutAt;
			string mypath = path;
			while( (cutAt = mypath.find("/")) != string::npos ){
				if(cutAt > 0)dirs.push_back(mypath.substr(0,cutAt));
				mypath = mypath.substr(cutAt+1);
			}
			if(mypath.length() > 0)dirs.push_back(mypath);	
		}
		
		hdef_t(){
			name = "";
			path = "";
			hnamepath = "";
			sum_hist=NULL;
			sum_hist_present = false;
		}
		
		bool operator== (const hdef_t& hi) const {
			return hi.hnamepath==hnamepath;
		}

		bool operator< (const hdef_t& rhs) const {
			return hnamepath == rhs.hnamepath;
		}


		string name;				// the name of the hist.
		string title;				// the title of the hist
		string path;				// the path where it can be found
		string hnamepath;			// path + "/" + name
		vector<string> dirs;		// directory elements of path
		histdimension_t type; 	// 2D, 3D, etc
		TH1* sum_hist;				// Pointer to sum histogram
		map<string, hinfo_t> hists;	// key is server name Pointer map to all summed hists in sum_hist
		bool sum_hist_modified;	// has the sum_hist histo been modified? (true when modified. false when drawn)
/*JustinB*/     bool sum_hist_present; //for SaveHists() method in rs_mainframe class to know when sum_hists are ready.

		map<string, bool> servers;	// servers that can provide this histogram// key=server val=histo was
                                                                                                    //marked for display
		map<string, bool> save_servers;
		bool active;				// include histos with this definition in display?
		bool save_active;

		hdisplay_info_t display_info;    // various options that modify how the histogram is displayed
};

//typedef hdef_t::histdimension_t histdimension_t;

#endif // _hdef_t_

		
