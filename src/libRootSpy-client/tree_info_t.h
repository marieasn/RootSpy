// $Id$
//
//    File: tree_info_t.h
// Created: Tuesday August 11, 2010
// Creator: justin barry
//

#ifndef _tree_info_t_
#define _tree_info_t_

#include <vector>
#include <list>
#include <map>
#include <TDirectory.h>
#include <TH1.h>
#include <TTree.h>
#include <string>

using namespace std;

#include <pthread.h>

class server_info_t;
class TH1;

class tree_id_t{
	public:

		tree_id_t(string server, string name, string path){
			this->servername = server;
			this->tnamepath = path + "/" + name;
			this->name = name;
			this->path = path;
		}

		tree_id_t(){
			this->servername = "";
			this->tnamepath = "";
		}

		string tnamepath;			// path + "/" + name
		string servername;
		string name;
		string path;

		bool operator==(const tree_id_t &T) const {
			return T.tnamepath==tnamepath && T.servername==servername;
		}
		bool operator!=(const tree_id_t &T) const {
			return !((*this)==T);
		}
		bool operator<(const tree_id_t &T) const {
			if(T.servername==servername)return T.tnamepath<tnamepath;
			return T.servername<servername;
		}
};

class tree_info_t:public tree_id_t{
	public:

		tree_info_t(string server, string name, string path, vector<string> &branch_info):
			tree_id_t(server, name, path){
				received = 0;
				tree = NULL;
				//active = true;
				//hasBeenDisplayed = false;
				isDisplayed = false;
				this->branch_info = branch_info;
		}

		tree_info_t():tree_id_t("", "", ""){
			received = 0;
			tree = NULL;
			//active = false;
			//hasBeenDisplayed = false;
			isDisplayed = false;
		}

		bool operator== (const tree_info_t& T);

		time_t received;
		TTree* tree;

		//bool active;					// sees if the histogram is usable
		//bool hasBeenDisplayed;		// makes sure that each hist is not displayed more than once

		vector<string> branch_info;  // make sure this is filled?

		bool getDisplayed() {return isDisplayed;}
		void setDisplayed(bool newDisplay) {isDisplayed = newDisplay;}

	private:
		bool isDisplayed;				// true only when the "loop over all servers" button is down and it is drawn

};

#endif // _tree_info_t_


