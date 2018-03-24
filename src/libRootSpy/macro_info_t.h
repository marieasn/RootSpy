
#ifndef _macro_info_t_
#define _macro_info_t_

#include <string>
using namespace std;

#include "TH1.h"
#include "TTree.h"

class macro_info_t {
public:
	macro_info_t() {
		name = "None";
		path = "/";
		hnamepath = path + "/" + name;
		macro = "";
		version = 0;
	}
		

macro_info_t(string in_name, string in_path):name(in_name),path(in_path) {
		hnamepath = path + "/" + name;
		macro = "";
		version = 0;
	}

        // identifying information for the macro
        string name;
        string path;
	string hnamepath;
        
        string macro;  // the actual code we want to execute
	// should we keep a md5 hash of this?

        // the data the macro will act on
        vector<TH1*> histograms;
        //vector<TTree*> trees;   // for future consideration
        // should we add binary data too?
        
        int32_t version;   // version number for disambiguating between scripts, larger is newer
};




#endif // _macro_info_t_
