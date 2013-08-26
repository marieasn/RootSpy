// $Id$
//
//    File: final_hist_t.h
// Created: 
// Creator: 
//

#ifndef _final_hist_t_
#define _final_hist_t_

#include <string>

class TH1;

/// in final_hist_t, we save the minimal amount of information to identify a histogram
// and where it should be saved

class final_hist_t{

 public:
    //final_hist_t(server_info_t &inServer,
    final_hist_t(string inServerName,
		 string inHnamepath,
		 TH1 *inHist) {
	//server = &inServer;
	serverName = inServerName;
	hnamepath = inHnamepath;
	//hist = new TH1(*inHist);
	hist = (TH1*)inHist->Clone();
	hist->SetDirectory(0);
    }

    //server_info_t *server;
    string serverName;
    string hnamepath;
    TH1* hist;
};

#endif // _final_hist_t_
