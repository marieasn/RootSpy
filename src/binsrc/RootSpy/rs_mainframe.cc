#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <fstream>
using namespace std;


#include "RootSpy.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"
#include "Dialog_SelectHists.h"
#include "Dialog_SaveHists.h"
#include "Dialog_IndivHists.h"
#include "Dialog_SelectTree.h"

#include <TROOT.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TPolyMarker.h>
#include <TLine.h>
#include <TMarker.h>
#include <TBox.h>
#include <TVector3.h>
#include <TGeoVolume.h>
#include <TGeoManager.h>
#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TBrowser.h>
#include <TArrow.h>
#include <TLatex.h>
#include <TColor.h>
#include <TGFileDialog.h>
#include <TFile.h>
#include <TGaxis.h>
#include <TFrame.h>

#include <KeySymbols.h>


extern string ROOTSPY_UDL;
extern string CMSG_NAME;



// globals for drawing histograms
static TH1 *overlay_hist = NULL;
static TGaxis *overlay_yaxis = NULL;


// information for menu bar
enum MenuCommandIdentifiers {
  M_FILE_OPEN,
  M_FILE_SAVE,
  M_FILE_EXIT,

  M_TOOLS_TBROWSER,
  M_TOOLS_TREEINFO,
  M_TOOLS_SAVEHISTS
};

//-------------------
// Constructor
//-------------------
rs_mainframe::rs_mainframe(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	// Define all of the -graphics objects. 
	CreateGUI();
	// Set up timer to call the D1oTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "rs_mainframe", this, "DoTimer()");
	sleep_time = 250;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL);
	last_called=now-1;
	last_ping_time = now;
	last_hist_requested = now -3;
	
	delay_time = 4; // default is 4 seconds (needs to be tied to default used to set GUI)
	
	last_requested.hnamepath = "N/A";
	last_hist_plotted = NULL;
	
	dialog_selectserverhist = NULL;
	dialog_selecthists = NULL;
	dialog_savehists = NULL;
	dialog_selecttree = NULL;
	dialog_indivhists = NULL;
	delete_dialog_selectserverhist = false;
	delete_dialog_selecthists = false;
	delete_dialog_savehists = false;
	can_view_indiv = false;

	//overlay_mode = false;
	archive_file = NULL;

	//overlay_mode = true;
	//archive_file = new TFile("/u/home/sdobbs/test_archives/run1_output.root");

	SetWindowName("RootSpy - Online");
	/** -- remove offline functionality for now
	// Finish up and map the window
	if(RS_CMSG->IsOnline())
	    SetWindowName("RootSpy - Online");
	else
	    SetWindowName("RootSpy - Offline");
	**/

	SetIconName("RootSpy");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

	viewStyle_rs = kViewByServer_rs;


	// Set ROOT style parameters
	//gROOT->SetStyle("Plain");
}
void rs_mainframe::CloseWindow(void) {
	DeleteWindow();
//	DestroyWindow();
	gApplication->Terminate(0);
}
//-------------------
// DoQuit
//-------------------
void rs_mainframe::DoQuit(void)
{
	cout<<"quitting ..."<<endl;
	SavePreferences();

	// This is supposed to return from the Run() method in "main()"
	// since we call SetReturnFromRun(true), but it doesn't seem to work.
	gApplication->Terminate(0);	
}

//-------------------
// MakeTB
//-------------------
void rs_mainframe::MakeTB(void)
{
	cout<<"Making new TBrowser"<<endl;
	TBrowser *TB = new TBrowser();
		
	//Outputs a new TBrowser, which will help with DeBugging.
}

//-------------------
// DoTimer
//-------------------
void rs_mainframe::DoTimer(void) {
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.

	// The following made the "View Indiv." button behave poorly
	// (not respond half the time it was clicked.) I've disabled this
	// for now to get it to behave better.  9/16/2010  D.L.
	//
	//Set indiv button enabled or disabled, added justin b.
	//if (can_view_indiv) indiv->SetEnabled(kTRUE);
	//else indiv->SetEnabled(kFALSE);

  // disable whole timer routine if we're not connected to cMsg? - sdobbs, 4/22/2013
        if(!RS_CMSG->IsOnline())
	    return;

	time_t now = time(NULL);
	
	// Pings server to keep it alive
	if(now-last_ping_time >= 3){
		RS_CMSG->PingServers();
		last_ping_time = now;
	}

	//selecthists dialog
	if(delete_dialog_selecthists) {
		dialog_selecthists = NULL;
	}
	delete_dialog_selecthists = false;

	//savehists dialog
	if(delete_dialog_savehists) {
		dialog_savehists = NULL;
	}
	delete_dialog_savehists = false;
	
	//indivhists dialog
	if(delete_dialog_indivhists) {
		dialog_indivhists = NULL;
	}
	delete_dialog_indivhists = false;

	//selecttree_dialog
	if(delete_dialog_selecttree){
		dialog_selecttree = NULL;
	}
	delete_dialog_selecttree = false;

	// Update server label if necessary
	if(selected_server){
		string s = selected_server->GetTitle();
		if(RS_INFO->servers.size() == 0){
			s = "No servers";
		} else if(RS_INFO->servers.size() == 1){
			s = RS_INFO->current.serverName;
		} else if(RS_INFO->servers.size() > 1){
			stringstream ss;
			ss << RS_INFO->servers.size();
			s = "("; 
			s += ss.str();  
			s += " servers)";
		}
		selected_server->SetText(TString(s));
	}
	
	// Update histo label if necessary
	if(selected_hist){
		string s = selected_hist->GetTitle();
		if(s!=RS_INFO->current.hnamepath){
			selected_hist->SetText(RS_INFO->current.hnamepath.c_str());
		}
	}
	// Update histo label if necessary
	if(retrieved_lab){
		string s = "";
		if(RS_INFO->servers.size() == 0){
			s = "No servers";
		} else if(RS_INFO->servers.size() == 1){
			s = "One server: " + RS_INFO->current.serverName;
		} else if(RS_INFO->servers.size() > 1 && loop_over_servers->GetState() != kButtonDown){
			s = "Several servers: ";
			map<string, hdef_t>::iterator h_iter = RS_INFO->histdefs.begin();
			for(; h_iter != RS_INFO->histdefs.end(); h_iter++){
				if(h_iter->second.active) {
					map<string, bool>::iterator s_iter = h_iter->second.servers.begin();
					for(; s_iter != h_iter->second.servers.end(); s_iter++) {
						if(s_iter->second){
							s += s_iter->first;
							s += " - ";	
						}
					}
				}

			}
		}
		retrieved_lab->SetText(TString(s));
	}

	
	// Request histogram update if auto button is on. If either of the
	// "Loop over servers" or "Loop over hists" boxes are checked, we
	// call DoNext(), otherwise call DoUpdate(). If time the auto_refresh
	// time has not expired but the last requested hist is not what is
	// currently displayed, then go ahead and call DoUpdate() to make
	// the display current.
	if(auto_refresh->GetState()==kButtonDown){
		if((now-last_hist_requested) >= delay_time){
			if(loop_over_servers->GetState()==kButtonDown) DoNext();
			else if(loop_over_hists->GetState()==kButtonDown) DoNext();
			else DoUpdate();
		}else if(last_requested!=RS_INFO->current){
			DoUpdate();
		}
	}
	
	// Request update of currently displayed histo if update flag is set
	if(RS_INFO->update)DoUpdate();
	
	// Redraw histo if necessary
	RS_INFO->Lock();
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	
	if(loop_over_servers->GetState()==kButtonDown && RS_INFO->servers.size() > 1 && hdef_iter->second.hists.size() > 0){
		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();
		while(!hinfo_it->second.getDisplayed() && hinfo_it != hdef_iter->second.hists.end()) {
				hinfo_it++;
		}
		
		if(hinfo_it != hdef_iter->second.hists.end() && !hinfo_it->second.hasBeenDisplayed){
		        canvas->cd();		
			if(hinfo_it->second.hist != NULL) {
			    //_DBG_ << "Pointer to histogram was not NULL" << endl;
			    //hinfo_it->second.hist->Draw();
			    DrawHist(hinfo_it->second.hist, hinfo_it->second.hnamepath,
				     hdef_iter->second.type);  
			} else {
				//_DBG_ << "Pointer to histogram was NULL" << endl;
			}
			canvas->Modified();	
			canvas->Update();
			hinfo_it->second.hasBeenDisplayed = true;
		}
	} else if(hdef_iter!=RS_INFO->histdefs.end()){
		if(hdef_iter->second.sum_hist_modified && hdef_iter->second.sum_hist!=NULL){
			canvas->cd();
			//hdef_iter->second.sum_hist->Draw();
			DrawHist(hdef_iter->second.sum_hist, hdef_iter->second.hnamepath,
				 hdef_iter->second.type);
			hdef_iter->second.sum_hist_modified = false;	// set flag indicating we've drawn current version
			canvas->Modified();
			canvas->Update();
		}
	}
	RS_INFO->Unlock();

	// Check when the last time we heard from each of the servers was
	// and delete any that we haven't heard from in a while.
	RS_INFO->Lock();
	map<string,server_info_t>::iterator iter=RS_INFO->servers.begin();	
	for(; iter!=RS_INFO->servers.end(); iter++){
		time_t &last_heard_from = iter->second.lastHeardFrom;
		if((now>=last_heard_from) && ((now-last_heard_from) > 10)){
			cout<<"server: "<<iter->first<<" has gone away"<<endl;
			RS_INFO->servers.erase(iter);
		}
	}
	RS_INFO->Unlock();

	last_called = now;
}

//-------------------
// DoSelectHists
//-------------------
void rs_mainframe::DoSelectHists(void)
{
	cout<<"entered DoSelectHists"<<endl;
	if(!dialog_selecthists){
		dialog_selecthists = new Dialog_SelectHists(gClient->GetRoot(), 10, 10);
	}else{
		dialog_selecthists->RaiseWindow();
		dialog_selecthists->RequestFocus();
	}
}

//-------------------
// DoSaveHists
//-------------------
void rs_mainframe::DoSaveHists(void)
{

	if(!dialog_savehists){
		dialog_savehists = new Dialog_SaveHists(gClient->GetRoot(), 10, 10);
	}else{
		dialog_savehists->RaiseWindow();
		dialog_savehists->RequestFocus();
	}
}

//-------------------
// DoIndiv
//-------------------
void rs_mainframe::DoIndiv(void) {
	if(!dialog_indivhists){
		dialog_indivhists = new Dialog_IndivHists(gClient->GetRoot(), 10, 10);
	}else{
		dialog_indivhists->RaiseWindow();
		dialog_indivhists->RequestFocus();
	}
}

//-------------------
// DoTreeInfo
//-------------------
void rs_mainframe::DoTreeInfo(void) {
	if(!dialog_selecttree){
		dialog_selecttree = new Dialog_SelectTree(gClient->GetRoot(), 10, 10);
	}else{
		dialog_selecttree->RaiseWindow();
		dialog_selecttree->RequestFocus();
	}
}

#if 0
//---------------------------------
// DoRadio
//---------------------------------
void rs_mainframe::DoRadio(void)
{
	time_t now = time(NULL);

	RS_INFO->Lock();

	map<string, server_info_t>::iterator siter = RS_INFO->servers.begin();
	for(; siter != RS_INFO->servers.end(); siter++)
	{
		vector<hinfo_t>::iterator hi = siter->second.hinfos.begin();
		if(hi==siter->second.hinfos.end())continue; // ignore servers for which we have no histogram info
		hi->hnamepath.clear();
		vector<hinfo_t> &hists = siter->second.hinfos;
		for(unsigned int j=0; j<hists.size(); j++){
			string hnamepath = hists[j].path;
			
			// Remove filename / root directory if present
			size_t pos = hnamepath.find(":");
			if(pos!=hnamepath.npos)hnamepath = hnamepath.substr(pos+1);
			
			if(hnamepath[hnamepath.length()-1]!='/')hnamepath += "/";
			hnamepath += hists[j].name;
			if(viewStyle_rs==kViewByObject_rs)
				hi->hnamepath = (hnamepath+"::"+siter->first);
			else
				hi->hnamepath = (siter->first+"::"+hnamepath);
		}
	}
	RS_INFO->Unlock();
}

//---------------------------------
// DoSetViewByObject
//---------------------------------
void rs_mainframe::DoSetVBObject(void)
{
	viewStyle_rs = kViewByObject_rs;
	VBObject->SetState(viewStyle_rs==kViewByObject_rs ? kButtonDown:kButtonUp);
	VBServer->SetState(viewStyle_rs==kViewByObject_rs ? kButtonUp:kButtonDown);

	DoRadio();
}

//---------------------------------
// DoSetViewByServer
//---------------------------------
void rs_mainframe::DoSetVBServer(void)
{
	viewStyle_rs = kViewByServer_rs;
	VBObject->SetState(viewStyle_rs==kViewByObject_rs ? kButtonDown:kButtonUp);
	VBServer->SetState(viewStyle_rs==kViewByObject_rs ? kButtonUp:kButtonDown);

	DoRadio();
}
#endif

//-------------------
// DoSelectDelay
//-------------------
void rs_mainframe::DoSelectDelay(Int_t index)
{
	_DBG_<<"Setting auto-refresh delay to "<<index<<"seconds"<<endl;
	delay_time = (time_t)index;
}

//------------------
// DoLoopOver
//------------------
void rs_mainframe::DoLoopOverServers(void)
{
	RS_INFO->Lock();
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	if(loop_over_servers->GetState()==kButtonUp && hdef_iter->second.hists.size() > 0){

		loop_over_hists->SetState(kButtonUp);
		
		_DBG_ << "I was here" << endl;	
			
		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();	
		hinfo_it->second.setDisplayed(true);

	} else {
		loop_over_servers->SetState(kButtonDown);
		loop_over_hists->SetState(kButtonUp);
	}	
		
	RS_INFO->Unlock();
}

//------------------
// DoLoopOverHists
//------------------
void rs_mainframe::DoLoopOverHists(void)
{
	RS_INFO->Lock();
		
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	if(loop_over_hists->GetState()==kButtonUp && hdef_iter->second.hists.size() > 0){
		loop_over_servers->SetState(kButtonUp);

		_DBG_ << "I was here" << endl;	

		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();	
		while(hinfo_it!=hdef_iter->second.hists.end()) {
			_DBG_ << "I was here" << endl;	
			hinfo_it->second.setDisplayed(false);	
			hinfo_it++;
			_DBG_ << "I was here" << endl;
		}	hinfo_it->second.setDisplayed(false);

	} else {
		loop_over_hists->SetState(kButtonDown);
		loop_over_servers->SetState(kButtonUp);
	}		
	RS_INFO->Unlock();
}


//-------------------
// DoUpdate
//-------------------
void rs_mainframe::DoUpdate(void)
{
	// Send a request to the server for an updated histogram

	RS_INFO->Lock();
	RS_INFO->update = false;
	
	bool request_sent = false;
	
	if(loop_over_servers->GetState()==kButtonDown){

		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();

		while(!hinfo_it->second.getDisplayed() && hinfo_it != hdef_iter->second.hists.end()) {
				hinfo_it++;
		}
		
		hinfo_it->second.hasBeenDisplayed = false;
			
		string &server = RS_INFO->current.serverName;
		string &hnamepath = RS_INFO->current.hnamepath;
		if(server!="" && hnamepath!=""){
			RS_CMSG->RequestHistogram(server, hnamepath);
			request_sent = true;
		}

	}else if(RS_INFO->viewStyle==rs_info::kViewByObject){
		// Loop over servers requesting this hist from each one (that is active)
		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
		if(hdef_iter!=RS_INFO->histdefs.end()){
			map<string, bool> &servers = hdef_iter->second.servers;
			map<string, bool>::iterator server_iter = servers.begin();
			for(; server_iter!=servers.end(); server_iter++){
				if(server_iter->second){
					RS_CMSG->RequestHistogram(server_iter->first, RS_INFO->current.hnamepath);
					request_sent = true;
				}
			}
		}
	}else{
		// Request only a single histogram
		string &server = RS_INFO->current.serverName;
		string &hnamepath = RS_INFO->current.hnamepath;
		if(server!="" && hnamepath!=""){
			RS_CMSG->RequestHistogram(server, hnamepath);
			request_sent = true;
		}
	}
	
	if(request_sent){
		time_t now = time(NULL);
		last_hist_requested = now;
		last_requested = RS_INFO->current;
	}

	RS_INFO->Unlock();
}

//-------------------
// DoNext
//-------------------
void rs_mainframe::DoNext(void)
{
	/// Get the next active histogram and request it from the server
	RS_INFO->Lock();

	// First, set the sum_hist_modified flag of the currently displayed
	// histo (if any) to true. This will ensure that when we come back to
	// this one, it will be redrawn.
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	
	if(loop_over_servers->GetState()==kButtonDown && RS_INFO->servers.size() > 1){
		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();
		while(hinfo_it!=hdef_iter->second.hists.end() && !hinfo_it->second.getDisplayed()){
			hinfo_it++;
		}
		if(hinfo_it->second.getDisplayed()){
				hinfo_it->second.setDisplayed(false);
				if(hinfo_it == hdef_iter->second.hists.end()){
					hinfo_it = hdef_iter->second.hists.begin();
				} else {
					hinfo_it++;
				}			
		} else {
			hinfo_it = hdef_iter->second.hists.begin();
		}
		hinfo_it->second.setDisplayed(true);	
	} else {
		if(hdef_iter!=RS_INFO->histdefs.end())hdef_iter->second.sum_hist_modified=true;
		RS_INFO->current = RS_INFO->FindNextActive(RS_INFO->current);
	}
		
	RS_INFO->Unlock();
	DoUpdate();
}

//-------------------
// DoPrev
//-------------------
void rs_mainframe::DoPrev(void)
{
	/// Get the previous active histogram and request it from the server

	RS_INFO->Lock();

	// First, set the sum_hist_modified flag of the currently displayed
	// histo (if any) to true. This will ensure than when we come back to
	// this one, it will be redrawn.
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	
	if(loop_over_servers->GetState()==kButtonDown && RS_INFO->servers.size() > 1){
		map<string, hinfo_t>::iterator hinfo_it = hdef_iter->second.hists.begin();
		map<string, hinfo_t>::iterator hinfo_it_plusone = hinfo_it; 
		hinfo_it_plusone++;
		while(hinfo_it!=hdef_iter->second.hists.end() || (hinfo_it_plusone)->second.getDisplayed()){
			hinfo_it++;
			hinfo_it_plusone++;
		}
		if((hinfo_it_plusone)->second.getDisplayed()){
				(hinfo_it_plusone)->second.setDisplayed(false);
				if(hinfo_it!=hdef_iter->second.hists.begin()){
					hinfo_it = hdef_iter->second.hists.end();
				}
		} else {
			hinfo_it == hdef_iter->second.hists.begin();
		}
		hinfo_it->second.setDisplayed(true);	
	} else {
		if(hdef_iter!=RS_INFO->histdefs.end())hdef_iter->second.sum_hist_modified=true;
		RS_INFO->current = RS_INFO->FindPreviousActive(RS_INFO->current);
	}

	RS_INFO->Unlock();

	DoUpdate();
}

//----------
//DoSave
//----------
//added by JustinBarry 06.14.10
//Save the current canvas as an image file.
void rs_mainframe::DoSave(void) {
	TGFileInfo* fileinfo = new TGFileInfo();
	TGFileDialog* filedialog = new TGFileDialog(gClient->GetRoot(), gClient->GetRoot(), kFDSave, fileinfo);
	canvas->SaveAs(fileinfo->fFilename, "");	
}

//----------
//DoSetArchiveFile
//----------
//add comment
void rs_mainframe::DoSetArchiveFile(void) {
	TGFileInfo* fileinfo = new TGFileInfo();
	TGFileDialog* filedialog = new TGFileDialog(gClient->GetRoot(), gClient->GetRoot(), kFDOpen, fileinfo);

	if(archive_file)
	    archive_file->Close();

	archive_file = new TFile(fileinfo->fFilename);
}

//-------------------
// ReadPreferences
//-------------------
void rs_mainframe::ReadPreferences(void)
{
	// Preferences file is "${HOME}/.RootSys"
	const char *home = getenv("HOME");
	if(!home)return;
	
	// Try and open file
	string fname = string(home) + "/.RootSys";
	ifstream ifs(fname.c_str());
	if(!ifs.is_open())return;
	cout<<"Reading preferences from \""<<fname<<"\" ..."<<endl;
	
	// Loop over lines
	char line[1024];
	while(!ifs.eof()){
		ifs.getline(line, 1024);
		if(strlen(line)==0)continue;
		if(line[0] == '#')continue;
		string str(line);
		
		// Break line into tokens
		vector<string> tokens;
		string buf; // Have a buffer string
		stringstream ss(str); // Insert the string into a stream
		while (ss >> buf)tokens.push_back(buf);
		if(tokens.size()<1)continue;

#if 0		
		// Check first token to decide what to do
		if(tokens[0] == "checkbutton"){
			if(tokens.size()!=4)continue; // should be of form "checkbutton name = value" with white space on either side of the "="
			map<string, TGCheckButton*>::iterator it = checkbuttons.find(tokens[1]);
			if(it != checkbuttons.end()){
				if(tokens[3] == "on")(it->second)->SetState(kButtonDown);
			}
		}
		
		if(tokens[0] == "DTrackCandidate"){
			if(tokens.size()!=3)continue; // should be of form "DTrackCandidate = tag" with white space on either side of the "="
			default_candidate = tokens[2];
		}

		if(tokens[0] == "DTrack"){
			if(tokens.size()!=3)continue; // should be of form "DTrack = tag" with white space on either side of the "="
			default_track = tokens[2];
		}

		if(tokens[0] == "DParticle"){
			if(tokens.size()!=3)continue; // should be of form "DParticle = tag" with white space on either side of the "="
			default_track = tokens[2];
		}

		if(tokens[0] == "Reconstructed"){
			if(tokens.size()!=3)continue; // should be of form "Reconstructed = Factory:tag" with white space on either side of the "="
			default_reconstructed = tokens[2];
		}
#endif		
	}
	
	// close file
	ifs.close();
}

//-------------------
// SavePreferences
//-------------------
void rs_mainframe::SavePreferences(void)
{
	// Preferences file is "${HOME}/.RootSys"
	const char *home = getenv("HOME");
	if(!home)return;
	
	// Try deleting old file and creating new file
	string fname = string(home) + "/.RootSys";
	unlink(fname.c_str());
	ofstream ofs(fname.c_str());
	if(!ofs.is_open()){
		cout<<"Unable to create preferences file \""<<fname<<"\"!"<<endl;
		return;
	}
	
	// Write header
	time_t t = time(NULL);
	ofs<<"##### RootSys preferences file ###"<<endl;
	ofs<<"##### Auto-generated on "<<ctime(&t)<<endl;
	ofs<<endl;

#if 0
	// Write all checkbuttons that are "on"
	map<string, TGCheckButton*>::iterator iter;
	for(iter=checkbuttons.begin(); iter!=checkbuttons.end(); iter++){
		TGCheckButton *but = iter->second;
		if(but->GetState() == kButtonDown){
			ofs<<"checkbutton "<<(iter->first)<<" = on"<<endl;
		}
	}
	ofs<<endl;

	ofs<<"DTrackCandidate = "<<(candidatesfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"DTrack = "<<(tracksfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"DParticle = "<<(particlesfactory->GetTextEntry()->GetText())<<endl;
	ofs<<"Reconstructed = "<<(reconfactory->GetTextEntry()->GetText())<<endl;
#endif

	ofs<<endl;
	ofs.close();
	cout<<"Preferences written to \""<<fname<<"\""<<endl;
}

//-------------------
// DoTreeInfo
//-------------------
void rs_mainframe::DoTreeInfoShort(void) {
	RS_INFO->Lock();
	map<string,server_info_t>::iterator iter = RS_INFO->servers.begin();
	for(; iter!=RS_INFO->servers.end(); iter++){
		string servername = iter->first;
		if(servername!=""){
			RS_CMSG->RequestTreeInfo(servername);
		}
	}
	RS_INFO->Unlock();
}

//-------------------
// CreateGUI
//-------------------
void rs_mainframe::CreateGUI(void)
{
	// Use the "color wheel" rather than the classic palette.
	TColor::CreateColorWheel();
	
	TGMainFrame *fMainFrame1435 = this;

   //==============================================================================================
   // make a menubar
   // Create menubar and popup menus. The hint objects are used to place
   // and group the different menu widgets with respect to eachother.
	//fMenuDock = new TGDockableFrame(fMainFrame1435, -1, kRaisedFrame );
	//fMainFrame1435->AddFrame(fMenuDock, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
	//fMenuDock->SetWindowName("RootSpy Menu");

   fMenuBarLayout = new TGLayoutHints(kLHintsTop | kLHintsExpandX);
   fMenuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
   //fMenuBarHelpLayout = new TGLayoutHints(kLHintsTop | kLHintsRight);
   
   fMenuFile = new TGPopupMenu(gClient->GetRoot());
   fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
   fMenuFile->AddEntry("&Save...", M_FILE_SAVE);
   fMenuFile->AddSeparator();
   fMenuFile->AddEntry("E&xit", M_FILE_EXIT);

   fMenuFile->DisableEntry(M_FILE_OPEN);
   fMenuFile->DisableEntry(M_FILE_SAVE);

   fMenuTools = new TGPopupMenu(gClient->GetRoot());
   fMenuTools->AddEntry("Start TBrowser", M_TOOLS_TBROWSER);
   fMenuTools->AddEntry("View Tree Info", M_TOOLS_TREEINFO);
   fMenuTools->AddEntry("Save Hists...",  M_TOOLS_SAVEHISTS);
   //fMenuTools->AddSeparator();
   

   //fMenuBar = new TGMenuBar(fMenuDock, 1, 1, kHorizontalFrame);
   fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame | kRaisedFrame );
   this->AddFrame(fMenuBar, fMenuBarLayout);
   //fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame | kRaisedFrame | kLHintsExpandX);
   //fMenuBar = new TGMenuBar(fMainFrame1435, 1, 1, kHorizontalFrame | kRaisedFrame | kLHintsExpandX);
   fMenuBar->AddPopup("&File", fMenuFile, fMenuBarItemLayout);
   fMenuBar->AddPopup("&Tools", fMenuTools, fMenuBarItemLayout);

   //fMenuDock->AddFrame(fMenuBar, fMenuBarLayout);

   //fMenuDock->EnableUndock(kTRUE);
   //fMenuDock->EnableHide(kTRUE);

   // When using the DockButton of the MenuDock,
   // the states 'enable' and 'disable' of menus have to be updated.
   //fMenuDock->Connect("Undocked()", "TestMainFrame", this, "HandleMenu(=M_VIEW_UNDOCK)");


   // connect the menus to methods
   // Menu button messages are handled by the main frame (i.e. "this")
   // HandleMenu() method.
   fMenuFile->Connect("Activated(Int_t)", "rs_mainframe", this, "HandleMenu(Int_t)");

	//======================= The following was copied for a macro made with TGuiBuilder ===============
	   // composite frame
   TGCompositeFrame *fMainFrame656 = new TGCompositeFrame(fMainFrame1435,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame657 = new TGCompositeFrame(fMainFrame656,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame658 = new TGCompositeFrame(fCompositeFrame657,833,700,kVerticalFrame);

   // composite frame
   TGCompositeFrame *fCompositeFrame659 = new TGCompositeFrame(fCompositeFrame658,833,700,kVerticalFrame);
//   fCompositeFrame659->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame660 = new TGCompositeFrame(fCompositeFrame659,833,700,kVerticalFrame);
//   fCompositeFrame660->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fCompositeFrame661 = new TGCompositeFrame(fCompositeFrame660,833,700,kVerticalFrame);
//   fCompositeFrame661->SetLayoutBroken(kTRUE);

   // vertical frame
   TGVerticalFrame *fVerticalFrame662 = new TGVerticalFrame(fCompositeFrame661,708,640,kVerticalFrame);
//   fVerticalFrame662->SetLayoutBroken(kTRUE);

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame663 = new TGHorizontalFrame(fVerticalFrame662,564,118,kHorizontalFrame);

   // "Current Histogram Info." group frame
   TGGroupFrame *fGroupFrame664 = new TGGroupFrame(fHorizontalFrame663,"Current Histogram Info.",kHorizontalFrame | kRaisedFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame665 = new TGVerticalFrame(fGroupFrame664,62,60,kVerticalFrame);
   TGLabel *fLabel666 = new TGLabel(fVerticalFrame665,"Server:");
   fLabel666->SetTextJustify(kTextRight);
   fLabel666->SetMargins(0,0,0,0);
   fLabel666->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel666, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   TGLabel *fLabel667 = new TGLabel(fVerticalFrame665,"Histogram:");
   fLabel667->SetTextJustify(kTextRight);
   fLabel667->SetMargins(0,0,0,0);
   fLabel667->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel667, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   TGLabel *fLabel668 = new TGLabel(fVerticalFrame665,"Retrieved:");
   fLabel668->SetTextJustify(kTextRight);
   fLabel668->SetMargins(0,0,0,0);
   fLabel668->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fLabel668, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));


   TGTextButton *fTextButtonIndiv = new TGTextButton(fVerticalFrame665,"View Indiv.");
   fTextButtonIndiv->SetTextJustify(36);
   fTextButtonIndiv->SetMargins(0,0,0,0);
   fTextButtonIndiv->SetWrapLength(-1);
   fTextButtonIndiv->Resize(200,22);
   fVerticalFrame665->AddFrame(fTextButtonIndiv, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

/*
   TGRadioButton *fRadioButton019 = new TGRadioButton(fVerticalFrame665,"View By Server");
   fRadioButton019->SetState(kButtonDown);
   fRadioButton019->SetTextJustify(36);
   fRadioButton019->SetMargins(0,0,0,0);
   fRadioButton019->SetWrapLength(-1);
   fVerticalFrame665->AddFrame(fRadioButton019, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
*/
   fGroupFrame664->AddFrame(fVerticalFrame665, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame669 = new TGVerticalFrame(fGroupFrame664,29,60,kVerticalFrame);
   TGLabel *fLabel670 = new TGLabel(fVerticalFrame669,"------------------------------");
   fLabel670->SetTextJustify(kTextLeft);
   fLabel670->SetMargins(0,0,0,0);
   fLabel670->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel670, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel671 = new TGLabel(fVerticalFrame669,"-------------------------------------------------------");
   fLabel671->SetTextJustify(kTextLeft);
   fLabel671->SetMargins(0,0,0,0);
   fLabel671->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel671, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGLabel *fLabel672 = new TGLabel(fVerticalFrame669,"-----test-------------------------");
   fLabel672->SetTextJustify(kTextLeft);
   fLabel672->SetMargins(0,0,0,0);
   fLabel672->SetWrapLength(-1);
   fVerticalFrame669->AddFrame(fLabel672, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame669, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame673 = new TGVerticalFrame(fGroupFrame664,97,78,kVerticalFrame);

   TGHorizontalFrame *fHorizontalFrame801 = new TGHorizontalFrame(fVerticalFrame673,97,24,kHorizontalFrame);
   TGTextButton *fTextButton674 = new TGTextButton(fHorizontalFrame801,"Select Server/Histo");
   fTextButton674->SetTextJustify(36);
   fTextButton674->SetMargins(0,0,0,0);
   fTextButton674->SetWrapLength(-1);
   fTextButton674->Resize(93,22);
   fHorizontalFrame801->AddFrame(fTextButton674, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fVerticalFrame673->AddFrame(fHorizontalFrame801, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   TGHorizontalFrame *fHorizontalFrame802 = new TGHorizontalFrame(fVerticalFrame673,97,24,kHorizontalFrame);
   TGTextButton *fTextButton677 = new TGTextButton(fHorizontalFrame802,"Prev");
   fTextButton677->SetTextJustify(36);
   fTextButton677->SetMargins(0,0,0,0);
   fTextButton677->SetWrapLength(-1);
   fTextButton677->Resize(87,22);
   fHorizontalFrame802->AddFrame(fTextButton677, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   TGTextButton *fTextButton675 = new TGTextButton(fHorizontalFrame802,"Next");
   fTextButton675->SetTextJustify(36);
   fTextButton675->SetMargins(0,0,0,0);
   fTextButton675->SetWrapLength(-1);
   fTextButton675->Resize(87,22);
   fHorizontalFrame802->AddFrame(fTextButton675, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fVerticalFrame673->AddFrame(fHorizontalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   TGHorizontalFrame *fHorizontalFrame803 = new TGHorizontalFrame(fVerticalFrame673,97,24,kHorizontalFrame);
   TGTextButton *fTextButton676 = new TGTextButton(fHorizontalFrame803,"Update");
   fTextButton676->SetTextJustify(36);
   fTextButton676->SetMargins(0,0,0,0);
   fTextButton676->SetWrapLength(-1);
   fTextButton676->Resize(87,22);
   fHorizontalFrame803->AddFrame(fTextButton676, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fVerticalFrame673->AddFrame(fHorizontalFrame803, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
//   TGTextButton *fSelectTree = new TGTextButton(fVerticalFrame673, "Tree Info");
//   fSelectTree->SetTextJustify(36);
//   fSelectTree->SetMargins(0,0,0,0);
//   fSelectTree->SetWrapLength(-1);
//   fSelectTree->Resize(10,10);
//   fVerticalFrame673->AddFrame(fTextButton676, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->AddFrame(fVerticalFrame673, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame664->SetLayoutManager(new TGHorizontalLayout(fGroupFrame664));
   fGroupFrame664->Resize(232,114);
   fHorizontalFrame663->AddFrame(fGroupFrame664, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // "fGroupFrame746" group frame
   TGGroupFrame *fGroupFrame746 = new TGGroupFrame(fHorizontalFrame663,"Continuous Update options",kHorizontalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame678 = new TGVerticalFrame(fGroupFrame746,144,63,kVerticalFrame);
   TGCheckButton *fCheckButton679 = new TGCheckButton(fVerticalFrame678,"Auto-refresh");
   fCheckButton679->SetTextJustify(36);
   fCheckButton679->SetMargins(0,0,0,0);
   fCheckButton679->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton679, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGCheckButton *fCheckButton680 = new TGCheckButton(fVerticalFrame678,"loop over all servers");
   fCheckButton680->SetTextJustify(36);
   fCheckButton680->SetMargins(0,0,0,0);
   fCheckButton680->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton680, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGCheckButton *fCheckButton681 = new TGCheckButton(fVerticalFrame678,"loop over all hists");
   fCheckButton681->SetTextJustify(36);
   fCheckButton681->SetMargins(0,0,0,0);
   fCheckButton681->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton681, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGCheckButton *fCheckButton6681 = new TGCheckButton(fVerticalFrame678,"show archived hists");
   fCheckButton6681->SetTextJustify(36);
   fCheckButton6681->SetMargins(0,0,0,0);
   fCheckButton6681->SetWrapLength(-1);
   fVerticalFrame678->AddFrame(fCheckButton6681, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame746->AddFrame(fVerticalFrame678, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame682 = new TGHorizontalFrame(fGroupFrame746,144,26,kHorizontalFrame);
   TGLabel *fLabel683 = new TGLabel(fHorizontalFrame682,"delay:");
   fLabel683->SetTextJustify(36);
   fLabel683->SetMargins(0,0,0,0);
   fLabel683->SetWrapLength(-1);
   fHorizontalFrame682->AddFrame(fLabel683, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   ULong_t ucolor;        // will reflect user color changes
   gClient->GetColorByName("#ffffff",ucolor);

   // combo box
   TGComboBox *fComboBox684 = new TGComboBox(fHorizontalFrame682,"-",-1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   fComboBox684->AddEntry("0s",0);
   fComboBox684->AddEntry("1s",1);
   fComboBox684->AddEntry("2s",2);
   fComboBox684->AddEntry("3s",3);
   fComboBox684->AddEntry("4s",4);
   fComboBox684->AddEntry("10s ",10);
   fComboBox684->Resize(50,22);
   fComboBox684->Select(-1);
   fHorizontalFrame682->AddFrame(fComboBox684, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame746->AddFrame(fHorizontalFrame682, new TGLayoutHints(kLHintsNormal));

   fGroupFrame746->SetLayoutManager(new TGHorizontalLayout(fGroupFrame746));
   fGroupFrame746->Resize(324,99);
   fHorizontalFrame663->AddFrame(fGroupFrame746, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fVerticalFrame662->AddFrame(fHorizontalFrame663, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fHorizontalFrame663->MoveResize(2,2,564,118);

   // embedded canvas
   TRootEmbeddedCanvas *fRootEmbeddedCanvas698 = new TRootEmbeddedCanvas(0,fVerticalFrame662,704,432);
   Int_t wfRootEmbeddedCanvas698 = fRootEmbeddedCanvas698->GetCanvasWindowId();
   TCanvas *c125 = new TCanvas("c125", 10, 10, wfRootEmbeddedCanvas698);
   fRootEmbeddedCanvas698->AdoptCanvas(c125);
   fVerticalFrame662->AddFrame(fRootEmbeddedCanvas698, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));
   fRootEmbeddedCanvas698->MoveResize(2,124,704,432);

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame1060 = new TGHorizontalFrame(fVerticalFrame662,704,82,kHorizontalFrame);

   // "fGroupFrame750" group frame
   TGGroupFrame *fGroupFrame711 = new TGGroupFrame(fHorizontalFrame1060,"cMsg Info.");
   //TGLabel *fLabel1030 = new TGLabel(fGroupFrame711,"UDL = cMsg://127.0.0.1/cMsg/rootspy");
   string udl_label = "UDL = "+ROOTSPY_UDL;
   TGLabel *fLabel1030 = new TGLabel(fGroupFrame711,udl_label.c_str());
   fLabel1030->SetTextJustify(36);
   fLabel1030->SetMargins(0,0,0,0);
   fLabel1030->SetWrapLength(-1);
   fGroupFrame711->AddFrame(fLabel1030, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));


   TGHorizontalFrame *fHorizontalFrame690 = new TGHorizontalFrame(fGroupFrame711,400,50,kHorizontalFrame);
   /*
   // Get online button added by sdobbs 4/22/13
   TGTextButton *fTextButtonOnline = new TGTextButton(fHorizontalFrame690,"Online");
   //TGTextButton *fTextButtonOnline = new TGTextButton(fGroupFrame711,"Online");
   fTextButtonOnline->SetTextJustify(36);
   fTextButtonOnline->SetMargins(0,0,0,0);
   fTextButtonOnline->SetWrapLength(-1);
   fTextButtonOnline->Resize(150,22);
   fHorizontalFrame690->AddFrame(fTextButtonOnline, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fGroupFrame711->AddFrame(fTextButtonOnline, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fGroupFrame711->AddFrame(fTextButtonOnline, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
   */

   TGTextButton *fTextButton1041 = new TGTextButton(fHorizontalFrame690,"modify");
   //TGTextButton *fTextButton1041 = new TGTextButton(fGroupFrame711,"modify");
   fTextButton1041->SetTextJustify(36);
   fTextButton1041->SetMargins(0,0,0,0);
   fTextButton1041->SetWrapLength(-1);
   fTextButton1041->Resize(97,22);
   fHorizontalFrame690->AddFrame(fTextButton1041, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fGroupFrame711->AddFrame(fTextButton1041, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fHorizontalFrame690->SetLayoutManager(new TGHorizontalLayout(fHorizontalFrame690));
   fGroupFrame711->AddFrame(fHorizontalFrame690, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fGroupFrame711->SetLayoutManager(new TGVerticalLayout(fGroupFrame711));
   fGroupFrame711->Resize(133,78);
   fHorizontalFrame1060->AddFrame(fGroupFrame711, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   TGTextButton *fTextButton1075 = new TGTextButton(fHorizontalFrame1060,"Quit");
   fTextButton1075->SetTextJustify(36);
   fTextButton1075->SetMargins(0,0,0,0);
   fTextButton1075->SetWrapLength(-1);
   fTextButton1075->Resize(97,22);
   fHorizontalFrame1060->AddFrame(fTextButton1075, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
   /**
   TGTextButton *fTextButtonTB = new TGTextButton(fHorizontalFrame1060,"TBrowser");
   fTextButtonTB->SetTextJustify(36);
   fTextButtonTB->SetMargins(0,0,0,0);
   fTextButtonTB->SetWrapLength(-1);
   fTextButtonTB->Resize(150,22);
   fHorizontalFrame1060->AddFrame(fTextButtonTB, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
   **/
   //Save Canvas button added by Justinb 6.10.10
   TGTextButton *fTextButtonSave = new TGTextButton(fHorizontalFrame1060,"Save Canvas");
   fTextButtonSave->SetTextJustify(36);
   fTextButtonSave->SetMargins(0,0,0,0);
   fTextButtonSave->SetWrapLength(-1);
   fTextButtonSave->Resize(200,22);
   fHorizontalFrame1060->AddFrame(fTextButtonSave, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
   /**
   //Save Hists button added by Justinb 6.10.10
   TGTextButton *fTextButtonSaveHists = new TGTextButton(fHorizontalFrame1060,"Save Hists");
   fTextButtonSaveHists->SetTextJustify(36);
   fTextButtonSaveHists->SetMargins(0,0,0,0);
   fTextButtonSaveHists->SetWrapLength(-1);
   fTextButtonSaveHists->Resize(250,22);
   fHorizontalFrame1060->AddFrame(fTextButtonSaveHists, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));

   //Save Hists button added by Justinb 6.10.10
   TGTextButton *fTextButtonfinal = new TGTextButton(fHorizontalFrame1060,"finals");
   fTextButtonfinal->SetTextJustify(36);
   fTextButtonfinal->SetMargins(0,0,0,0);
   fTextButtonfinal->SetWrapLength(-1);
   fTextButtonfinal->Resize(300,22);
   fHorizontalFrame1060->AddFrame(fTextButtonfinal, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));
  
   TGTextButton *fTextButton1297 = new TGTextButton(fHorizontalFrame1060,"Tree Info");
   fTextButton1297->SetTextJustify(36);
   fTextButton1297->SetMargins(0,0,0,0);
   fTextButton1297->SetWrapLength(-1);
   fTextButton1297->Resize(97,22);
   fHorizontalFrame1060->AddFrame(fTextButton1297, new TGLayoutHints(kLHintsNormal));
   **/
   TGTextButton *fTextButtonSetArchive = new TGTextButton(fHorizontalFrame1060,"Set Archive");
   fTextButtonSetArchive->SetTextJustify(36);
   fTextButtonSetArchive->SetMargins(0,0,0,0);
   fTextButtonSetArchive->SetWrapLength(-1);
   fTextButtonSetArchive->Resize(300,22);
   fHorizontalFrame1060->AddFrame(fTextButtonSetArchive, new TGLayoutHints(kLHintsRight | kLHintsTop,2,2,2,2));


   fVerticalFrame662->AddFrame(fHorizontalFrame1060, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
   fHorizontalFrame1060->MoveResize(2,560,704,82);

   fCompositeFrame661->AddFrame(fVerticalFrame662, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fVerticalFrame662->MoveResize(2,2,708,640);

   fCompositeFrame660->AddFrame(fCompositeFrame661, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fCompositeFrame661->MoveResize(0,0,833,700);

   fCompositeFrame659->AddFrame(fCompositeFrame660, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fCompositeFrame660->MoveResize(0,0,833,700);

   fCompositeFrame658->AddFrame(fCompositeFrame659, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fCompositeFrame657->AddFrame(fCompositeFrame658, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame656->AddFrame(fCompositeFrame657, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame1435->AddFrame(fMainFrame656, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

   fMainFrame1435->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame1435->MapSubwindows();

   fMainFrame1435->Resize(fMainFrame1435->GetDefaultSize());
   fMainFrame1435->MapWindow();
   fMainFrame1435->Resize(833,700);



   //==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &quit = fTextButton1075;
	//TGTextButton* &TB = fTextButtonTB;
	TGTextButton* &save = fTextButtonSave; //added Justin.b 06.10.10
	//TGTextButton* &savehists = fTextButtonSaveHists; //added Justin.b 06.15.10
	TGTextButton* &selectserver = fTextButton674;
	//TGTextButton* &treeinfobutton = fTextButton1297;
	//TGTextButton* &final = fTextButtonfinal;
	TGTextButton* &setarchive = fTextButtonSetArchive;
	//TGTextButton* &online = fTextButtonOnline;
	indiv = fTextButtonIndiv;

	//this->VBServer = fRadioButton019;
	selected_server = fLabel670;
	selected_hist = fLabel671;
	retrieved_lab = fLabel672;
	delay = fComboBox684;
	canvas = fRootEmbeddedCanvas698->GetCanvas();
	auto_refresh = fCheckButton679;
	loop_over_servers = fCheckButton680;
	loop_over_hists = fCheckButton681;
	show_archived_hists = fCheckButton6681;
	TGTextButton* &update = fTextButton676;
	TGTextButton* &next = fTextButton675;
	TGTextButton* &prev = fTextButton677;
	TGTextButton* &modify = fTextButton1041;
	
	quit->Connect("Clicked()","rs_mainframe", this, "DoQuit()");

	//TB->Connect("Clicked()","rs_mainframe", this, "MakeTB()");
        save->Connect("Clicked()", "rs_mainframe", this, "DoSave()");//added Justin.b 06.10.10
	//savehists->Connect("Clicked()", "rs_mainframe", this, "DoSaveHists()");//added Justin.b 06.10.15
	selectserver->Connect("Clicked()","rs_mainframe", this, "DoSelectHists()");
	indiv->Connect("Clicked()","rs_mainframe", this, "DoIndiv()");
	//this->VBServer->Connect("Clicked()","rs_mainframe", this, "DoSetVBServer()");
	next->Connect("Clicked()","rs_mainframe", this, "DoNext()");
	prev->Connect("Clicked()","rs_mainframe", this, "DoPrev()");

	//online->Connect("Clicked()", "rs_mainframe", this, "DoOnline()");
	//final->Connect("Clicked()", "rs_mainframe", this, "DoFinal()");
	//treeinfobutton->Connect("Clicked()", "rs_mainframe", this, "DoTreeInfo()");
	setarchive->Connect("Clicked()", "rs_mainframe", this, "DoSetArchiveFile()");
	update->Connect("Clicked()","rs_mainframe", this, "DoUpdate()");
	delay->Select(4, kTRUE);
	delay->GetTextEntry()->SetText("4s");
	delay->Connect("Selected(Int_t)","rs_mainframe", this, "DoSelectDelay(Int_t)");
	
	loop_over_servers->Connect("Clicked()","rs_mainframe",this, "DoLoopOverServers()");
	loop_over_hists->Connect("Clicked()","rs_mainframe",this, "DoLoopOverHists()");
	
	modify->SetEnabled(kFALSE);
	
	canvas->SetFillColor(kWhite);
}


void rs_mainframe::HandleMenu(Int_t id)
{
   // Handle menu items.

   switch (id) {

   case M_FILE_EXIT: 
     DoQuit();       
     break;

   case M_TOOLS_TBROWSER:
     MakeTB();
     break;

   case M_TOOLS_TREEINFO:
     DoTreeInfo();
     break;

   case M_TOOLS_SAVEHISTS:    
     DoSaveHists();
     break;
   }
}


Bool_t rs_mainframe::HandleKey(Event_t *event)
{
  // Handle keyboard events.

  char   input[10];
   UInt_t keysym;

  cerr << "in HandleKey()..." << endl;

  if (event->fType == kGKeyPress) {
    gVirtualX->LookupString(event, input, sizeof(input), keysym);

    cerr << " key press!" << endl;

    /*
    if (!event->fState && (EKeySym)keysym == kKey_F5) {
      Refresh(kTRUE);
      return kTRUE;
    }
    */
    switch ((EKeySym)keysym) {   // ignore these keys
    case kKey_Shift:
    case kKey_Control:
    case kKey_Meta:
    case kKey_Alt:
    case kKey_CapsLock:
    case kKey_NumLock:
    case kKey_ScrollLock:
      return kTRUE;
    default:
      break;
    }


    if (event->fState & kKeyControlMask) {   // Cntrl key modifier pressed
      switch ((EKeySym)keysym & ~0x20) {   // treat upper and lower the same
      case kKey_X:
	fMenuFile->Activated(M_FILE_EXIT);
	return kTRUE;
      default:
	break;
      }
    }
  }
  return TGMainFrame::HandleKey(event);
}


void rs_mainframe::DoFinal(void) {
	//loop servers
	map<string, server_info_t>::iterator serviter = RS_INFO->servers.begin();
	for(; serviter != RS_INFO->servers.end(); serviter++) {
		string server = serviter->first;
		vector<string> paths = serviter->second.hnamepaths;		
		RS_CMSG->FinalHistogram(server, paths);
		
	}
}


void rs_mainframe::DoOnline(void)
{
    if(!RS_CMSG->IsOnline()) {
	// Create cMsg object
	char hostname[256];
	gethostname(hostname, 256);
	char str[512];
	sprintf(str, "RootSpy GUI %s-%d", hostname, getpid());
	CMSG_NAME = string(str);
	cout << "Full UDL is " << ROOTSPY_UDL << endl;

	delete RS_CMSG;
	RS_CMSG = new rs_cmsg(ROOTSPY_UDL, CMSG_NAME);
    }

    if(RS_CMSG->IsOnline())
	SetWindowName("RootSpy - Online");
    else
	SetWindowName("RootSpy - Offline");
}

/**
// helper function
// move this somewhere else?
static void add_to_draw_hist_args(string &args, string toadd) 
{
    // we have to be careful about where we put spaces since ROOT is really picky about argument formatting
    if(args == "") 
	args = toadd;
    else 
	args = args + " " + toadd;	    
}
**/

// wrapper for histogram drawing
void rs_mainframe::DrawHist(TH1 *hist, string hnamepath,
			    hdef_t::histdimension_t hdim )
{
    string histdraw_args = "";
    bool overlay_mode = (show_archived_hists->GetState()==kButtonDown);
    //double hist_line_width = 1.;
    float overlay_ymin=0., overlay_ymax=0.;
    
    
    if(hdim == hdef_t::histdimension_t::oneD) {
	bool do_overlay = false;
	//TH1 *archived_hist = NULL;

	// check for archived histograms and load them if they exist and we are overlaying
	// we do this first to make see how we should draw the summed histogram
	if(overlay_mode && (archive_file!=NULL) ) {
	    _DBG_<<"trying to get archived histogram: " << hnamepath << endl;
	    TH1* archived_hist = (TH1*)archive_file->Get(hnamepath.c_str());
	    
	    if(archived_hist) { 
		// only plot the archived histogram if we can find it
		_DBG_<<"found it!"<<endl;
		do_overlay = true;
	
		// only display a copy of the archived histogram so that we can 
		// compare to the original
		if(overlay_hist)
		    delete overlay_hist;
		overlay_hist = (TH1*)(archived_hist->Clone()); 

		overlay_hist->SetStats(0);  // we want to compare just the shape of the archived histogram
		overlay_hist->SetFillColor(5); // draw archived histograms with shading

		overlay_ymin = overlay_hist->GetMinimum();
		overlay_ymax = 1.1*overlay_hist->GetMaximum();

		//_DBG_ << " overlay max, min " << overlay_ymax << ", " << overlay_ymin << endl;
	    }
	}

	// draw summed histogram
	if(!do_overlay) {
	    hist->Draw();

	} else {
	    // set axis ranges so that we can sensibly overlay the histograms
	    double hist_ymax = 1.1*hist->GetMaximum();
	    double hist_ymin = hist->GetMinimum();
	    // make sure we at least go down to zero, so that we are displaying the whole
	    // distribution
	    if(hist_ymin > 0)
		hist_ymin = 0;
	    
	    hist->GetYaxis()->SetRangeUser(hist_ymin, hist_ymax); 

	    // draw the current histogram with points/error bars if overlaying
	    hist->SetMarkerStyle(20);
	    hist->SetMarkerSize(0.7);
	    
	    //hist->Draw("E1");
	    //hist->Draw("AXIS");

	    // scale down archived histogram and display it to set the scale
	    float scale = hist_ymax/overlay_ymax;
	    //_DBG_ << gPad->GetUymax() << " " << overlay_ymax << " " << scale << endl;

	    overlay_hist->Scale(scale);
	    overlay_hist->Draw();

	    // now print the current histogram on top of it
	    hist->Draw("SAME E1");
	    //hist->Draw("AXIS");

	    gStyle->SetStatX(0.85);
	    //cerr << gStyle->GetStatX() << "  " << gStyle->GetStatY() << endl;

	    // add axis to the right for scale of archived histogram
	    if(overlay_yaxis != NULL)
		delete overlay_yaxis;
	    overlay_yaxis = new TGaxis(gPad->GetUxmax(),hist_ymin,
				       gPad->GetUxmax(),hist_ymax,
				       overlay_ymin,overlay_ymax,510,"+L");
	    overlay_yaxis->SetLabelFont( hist->GetLabelFont() );
	    overlay_yaxis->SetLabelSize( hist->GetLabelSize() );
	    overlay_yaxis->Draw();
	}

    } else if(hdim == hdef_t::histdimension_t::twoD) {

	// handle drawing 2D histograms differently
	// probably don't want to overlay them
	hist->Draw("COLZ");   // do some sort of pretty printing
	
    } else {
	// default to drawing without arguments
	hist->Draw();
    }

}
