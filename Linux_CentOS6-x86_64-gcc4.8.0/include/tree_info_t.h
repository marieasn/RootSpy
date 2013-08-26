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

		bool operator==(const tree_id_t &h) const {
			return h.tnamepath==tnamepath && h.servername==servername;
		}
		bool operator!=(const tree_id_t &h) const {
			return !((*this)==h);
		}
		bool operator<(const tree_id_t &h) const {
			if(h.servername==servername)return h.tnamepath<tnamepath;
			return h.servername<servername;
		}
};

class tree_info_t:public tree_id_t{
	public:

		tree_info_t(string server, string name, string path, vector<string> &branch_info):
			tree_id_t(server, name, path){
				received = 0;
				hist = NULL;
				active = true;
				hasBeenDisplayed = false;
				isDisplayed = false;
				this->branch_info = branch_info;
		}

		tree_info_t():tree_id_t("", "", ""){
			received = 0;
			hist = NULL;
			active = false;
			hasBeenDisplayed = false;
			isDisplayed = false;
		}

		bool operator== (const tree_info_t& hi);

		time_t received;
		TH1* hist;

		bool active;					// sees if the histogram is usable
		bool hasBeenDisplayed;		// makes sure that each hist is not displayed more than once

		vector<string> branch_info;

		bool getDisplayed() {return isDisplayed;}
		void setDisplayed(bool newDisplay) {isDisplayed = newDisplay;}

	private:
		bool isDisplayed;				// true only when the "loop over all servers" button is down and it is drawn

};

#endif // _tree_info_t_


