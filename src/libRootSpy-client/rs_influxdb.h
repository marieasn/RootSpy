// $Id$
//
//    File: rs_influxdb.h
// Created: Tue Aug 29 09:36:37 EDT 2017
// Creator: davidl (on Linux gluon112.jlab.org 2.6.32-642.3.1.el6.x86_64 x86_64)
//
//
//  This class is used to make entries into the influxdb that stores
// time series data. We use it in GlueX to store values from the online
// monitoring system like the means and widths of fits to resonances.
// This allows drifts and trends to be seen during the run.
//
// Most of the heavy lifting is done by the libcurl library. To use this,
// instantiate an rs_influxdb object giving it the host, port, and database
// name. Then, just call the AddData method for each item you wish to
// write. AddData takes 3 arguments:
//
//  item name
//  map of tags (optional)
//  map of values
//
// The map of tags can be empty (though a reference to an empty container
// must still be passed in). The map of values has a templated type for the
// actual values so anything that can be converted using stringstream is
// allowed. The one limitation is that since it is a map, all values must
// be the same type which is not something srictly required by influxdb. If
// you wish to store fields with different types, make the template parameter
// a string type and convert them yourself. Influxdb will automatically
// determine the types from the stringified query form anyway.

#ifndef _rs_influxdb_
#define _rs_influxdb_

#include <map>
#include <mutex>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>
using namespace std;

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

class rs_influxdb{
	public:
		
		//---------------------
		// Constructor
		rs_influxdb(const string &host, uint32_t port, const string &database)
			:host(host),port(port),database(database){

			// Initialize CURL
			curl_global_init(CURL_GLOBAL_ALL); //  (bit dangerous when multi-threading)
			curl = curl_easy_init();

			// Make sure database exists.
			// Unable to get this working with libcurl so just do
			// it with curl CLI. The "-s" and "> /dev/null" masks 
			// any output which means any errors will also go silent.
			stringstream ss;
			ss << "curl -s -XPOST 'http://" << host << ":" << port << "/query' --data-urlencode \"q=CREATE DATABASE " << database << "\" >/dev/null";
			system(ss.str().c_str());

			// Replace url with one we'll use for all data writes
			ss.str("");
			ss << "http://" << host << ":" << port << "/write?db=" << database;
			url = ss.str();		
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			
		}

		//---------------------
		// Destructor
		virtual ~rs_influxdb(){
			curl_easy_cleanup(curl);
		}
		
		//---------------------
		// AddData
		template<typename T>
		int AddData(const string &item, const map<string, string> &tags, const map<string,T> &vals){

			lock_guard<mutex> lck(mtx); // make us thread safe

			// Form POST data
			stringstream ss;
			ss << item;
			for(auto t : tags) ss << "," << t.first << "=" << t.second;
			ss << " ";
			for(auto v : vals) ss << v.first << "=" << v.second << ",";
			string post_data = ss.str();
			post_data.pop_back(); // remove last ","

			// Send data to DB
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
			CURLcode res = curl_easy_perform(curl);
			if(res != CURLE_OK) cerr << curl_easy_strerror(res) << endl;
		}
		
		//................................................................................

		// member data
		string host;
		uint32_t port;
		string database;
		string url;
		CURL *curl;
		mutex mtx;
};


#endif // _rs_influxdb_

