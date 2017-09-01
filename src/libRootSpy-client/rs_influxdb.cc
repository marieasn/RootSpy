// $Id$
//
//    File: rs_influxdb.cc
// Created: Tue Aug 31 16:30:37 EDT 2017
// Creator: davidl (on Linux gluon112.jlab.org 2.6.32-642.3.1.el6.x86_64 x86_64)


#include "rs_influxdb.h"

#include <string.h>
#include <stdlib.h>

#include <sstream>
#include <iostream>


//=======================================================
// These globals are used by the global InsertSeriesData
// routines. Those are here to make it possible to write
// to the influxdb from a macro.
static rs_influxdb *db = NULL;
static bool enabled = true;
//=======================================================


//---------------------
// Constructor
//---------------------
rs_influxdb::rs_influxdb(const string &host, const string &database, uint32_t port)
	:host(host),database(database),port(port){

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
	ss << "http://" << host;
	if(port!=0) ss << ":" << port; // user may choose to include port in host
	ss << "/write?db=" << database;
	url = ss.str();		
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

}

//---------------------
// Destructor
//---------------------
rs_influxdb::~rs_influxdb(){
	curl_easy_cleanup(curl);
}

//---------------------
// AddData
//---------------------
template<typename T>
int rs_influxdb::AddData(const string &item, const map<string, string> &tags, const map<string,T> &vals, uint32_t t){

	// If user did not provide timestamp, then use system clock
	if(t==0) t = time(NULL);

	// Form POST data
	stringstream ss;
	ss << item;
	for(auto t : tags) ss << "," << t.first << "=" << t.second;
	ss << " ";
	for(auto v : vals) ss << v.first << "=" << v.second << ",";
	ss.str().pop_back(); // remove last ","
	ss << " " << t;

	// Send data to DB
	return AddData(ss.str());
}

//---------------------
// AddData
//---------------------
int rs_influxdb::AddData(const string &sdata){

	lock_guard<mutex> lck(mtx); // make us thread safe

	// Send data to DB
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sdata.c_str());
	CURLcode res = curl_easy_perform(curl);
	if(res != CURLE_OK) { cerr << curl_easy_strerror(res) << endl; return res; }

	return 0;
}


//=======================================================
// The following are global scope routines, primarily to
// make it easy to insert data from a macro where having
// the full rs_influxdb class defined would be hard.


//-------------------
// InitSeriesData
//-------------------
void InitSeriesData(void)
{
	// Check flag if we found there is not enough info to
	// connect to DB so subsequent calls return immediately
	if(!enabled) return;

	// Initialize rs_influxdb pointer. Do this only once
	// but in a thread safe way.
	if(!db){
	
		// Only one thread gets to initialize. Put lock
		// in here so we don't have to lock on subsequent
		// calls.
		static mutex mtx;
		lock_guard<mutex> lck(mtx);
		if(!db){
		
			const char *HDTSDB_HOST = getenv("HDTSDB_HOST");
			const char *HDTSDB_PORT = getenv("HDTSDB_PORT");
			const char *HDTSDB_DB   = getenv("HDTSDB_DB");

			string   dbhost   = HDTSDB_HOST ? HDTSDB_HOST:"";
			uint32_t dbport   = HDTSDB_PORT ? atoi(HDTSDB_PORT):0;
			string   database = HDTSDB_DB   ? HDTSDB_DB:"";
			
			if(dbhost.empty() || database.empty()) {
				cout << "HDTSDB_HOST or HDTSDB_DB env. vars not set." << endl;
				cout << "Time Series DB writing disabled" << endl;
				enabled = false;
				return;
			}else{
				cout << "Time Series enabled with:" << endl;
				cout << "   HDTSDB_HOST = " << dbhost << endl;
				if(dbport>0) cout << "   HDTSDB_PORT = " << dbport << endl;
				cout << "     HDTSDB_DB = " << database << endl;
			}
		
			db = new rs_influxdb(dbhost, database, dbport);
		}
	}
}

//-------------------
// InsertSeriesData
//-------------------
void InsertSeriesData(string sdata)
{
	if(!db) InitSeriesData();
	if(db){

		cout << "InsertSeriesData called with " << sdata << endl;

		db->AddData(sdata);
	}
}

//-------------------
// InsertSeriesData
//-------------------
void InsertSeriesData(string item, map<string, string> tags, map<string,double> vals, uint32_t t)
{
	if(!db) InitSeriesData();
	if(db){

		cout << "InsertSeriesData called with " << item << ", " << tags.size() << " tags, and " << vals.size() << " vals with t=" << t << endl;

		db->AddData(item, tags, vals, t);
	}
}

