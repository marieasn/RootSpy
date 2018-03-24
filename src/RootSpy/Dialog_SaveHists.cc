// $Id$
//
//    File: Dialog_SaveHists.cc
// Created: 06.21.10
// Creator: Justin B (I just copied the source code from Davidl's Dialog_SelectHists.cc and changed some things around)
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_SaveHists.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TTimer.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TGFileDialog.h>
#include <unistd.h>
#include <TObjString.h>
#include <TGProgressBar.h>

//---------------------------------
// Dialog_SaveHists    (Constructor)
//---------------------------------
Dialog_SaveHists::Dialog_SaveHists(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	// Define all of the of the graphics objects. 
	CreateGUI();
	// Set up timer to call the DoTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_SaveHists", this, "DoTimer()");
	sleep_time = 200;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL) - 3;
	last_called = now;
	last_ping_time = now;
	last_hist_time = now;
	
	//changed JustinB. 06.26.10
	viewStyle = rs_info::kViewByObject;
	tosave = false;
	displaybar = false;
	DoTimer();

	// Finish up and map the window
	SetWindowName("RootSpy Save Histogram");
	SetIconName("SelectHist");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

//---------------------------------
// ~Dialog_SaveHists    (Destructor)
//---------------------------------
Dialog_SaveHists::~Dialog_SaveHists()
{
//	delete timer;
}
void Dialog_SaveHists::CloseWindow(void) {
	timer->Stop();
	delete timer;
	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_savehists = true;
}

//-------------------
// DoTimer
//-------------------
void Dialog_SaveHists::DoTimer(void)
{
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	
	time_t now = time(NULL);

	// Ping servers occasionally to make sure our list is up-to-date
	if(now-last_ping_time >= 3){
		if( RS_CMSG) RS_CMSG->PingServers();
		if( RS_XMSG) RS_XMSG->PingServers();
		last_ping_time = now;
	}
	
	// Ping server occasionally to make sure our histogram list is up-to-date
	if(now-last_hist_time >= 3){
		// Loop over servers
		RS_INFO->Lock();
		map<string,server_info_t>::iterator iter = RS_INFO->servers.begin();
		for(; iter!=RS_INFO->servers.end(); iter++){
			string servername = iter->first;
			if(servername!=""){
				if( RS_CMSG) RS_CMSG->RequestHists(servername);
				if( RS_XMSG) RS_XMSG->RequestHists(servername);
			}
		}
		RS_INFO->Unlock();
		last_hist_time = now;
	}

	// Get list of active hnamepaths on active servers
	// We get a copy of the hinfo_t objects so we don't
	// have to hold a mutex lock to prevent them from
	// changing on us while we're working with them.
	vector<hid_t> hids;
	GetAllHistos(hids);

	// Check if list of histos has changed
	bool hists_changed = false;
	if(last_hids.size() == hids.size()){
		for(unsigned int i=0; i<hids.size(); i++){
			if(hists_changed)break;
			if(hids[i] !=last_hids[i])hists_changed=true;
		}
	}else{
		hists_changed = true;
	}

	// Refill TGListTree if needed
	if(hists_changed)UpdateListTree(hids);

	last_called = now;

	//Check if Histograms are present for the DoSave method
	if(tosave && AreHistosPresent()) {
		SaveHistograms();
	}
//	cout<<"displaybar "<<displaybar<<endl;
	// If the load progress bar has been added then start to progress it.
	if(displaybar) {
//		cout<<"displaybar true"<<endl;
		progressbar->Increment((progressbar->GetMax() + progressbar->GetPosition())/2);
	}
}


//---------------------------------
// DoOK
//---------------------------------
void Dialog_SaveHists::DoOK(void)
{	
	RS_INFO->Lock();	

	// Loop over server items and copy the check box status to the appropriate
	// place in RS_INFO. Specfically, if we are viewing by object, then the server
	// checkbox corresponds to the values in RS_INFO->histdefs.servers. If viewing
	// by server, then is corresponds to the active flag in RS_INFO->servers[i].
	map<hid_t, TGListTreeItem*>::iterator server_items_iter = server_items.begin();
	for(; server_items_iter!=server_items.end(); server_items_iter++){
		const hid_t &hid = server_items_iter->first;
		TGListTreeItem *item = server_items_iter->second;
		if(viewStyle==rs_info::kViewByObject){
			map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hid.hnamepath);
			hdef_iter->second.save_servers[hid.serverName] = item->IsChecked();
		}
//		else{
//			map<string,server_info_t>::iterator server_info_iter = RS_INFO->save_servers.find
//                                                                                                (hid.serverName);
//			server_info_iter->second.active = item->IsChecked();
//		}
	}

	// Similar to above except for histos. If viewing by object, then the value
	// goes into RS_INFO->histdefs[].active. If viewing by server, it corresponds 
	// to RS_INFO->histdefs.servers
	map<hid_t, TGListTreeItem*>::iterator hist_items_iter = hist_items.begin();
	for(; hist_items_iter!=hist_items.end(); hist_items_iter++){
	
		const hid_t &hid = hist_items_iter->first;
		TGListTreeItem *item = hist_items_iter->second;
		
		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hid.hnamepath);
		if(viewStyle==rs_info::kViewByObject){
			hdef_iter->second.save_active = item->IsChecked();
		}
//		else{
//			//hdef_iter->second.servers[hid.serverName] = item->IsChecked();
//		}
	}
/*
	// If the RS_INFO->current value is not set, then set it to the first server/histo
	// and set the flag to have DoUpdate called
	if(RS_INFO->save_servers.find(RS_INFO->current.serverName)==RS_INFO->save_servers.end()){
		if(RS_INFO->save_servers.size()>0){
			map<string,server_info_t>::iterator server_iter = RS_INFO->save_servers.begin();
			if(server_iter->second.hnamepaths.size()>0){
				RS_INFO->current = hid_t(server_iter->first, server_iter->second.hnamepaths[0]);
			}
		}
	}
*/	
	RS_INFO->viewStyle = viewStyle;
	//RS_INFO->update = true;


	RS_INFO->Unlock();
	
	//DoCancel();
}


//---------------------------------
// DoCancel
//---------------------------------
void Dialog_SaveHists::DoCancel(void)
{
	//_DBG_<<"CLICKED CANCEL!"<<endl<<endl;
	timer->Stop();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_savehists = true;
}

//---------------------------------
// DoClickedEntry
//---------------------------------
void Dialog_SaveHists::DoClickedEntry(TGListTreeItem* entry, Int_t btn)
{
	// Called whenever an item is selected in listTree

	char path[512];
	listTree->GetPathnameFromItem(entry, path);
	_DBG_<<"Clicked entry \""<<path<<"\" with btn="<<btn<<endl;
}

//---------------------------------
// DoCheckedEntry
//---------------------------------
void Dialog_SaveHists::DoCheckedEntry(TObject* obj, Int_t check)
{
	// Called whenever a checkbox is toggled in listTree
	_DBG_<<"Checkbox toggled to "<<check<<" for object 0x"<<hex<< "-" <<(unsigned long)obj<<dec<<endl;
	TGListTreeItem *item = (TGListTreeItem *)(obj);
	if(!item) return;

	// Loop over children of this item and set them to inactive
	TGListTreeItem *child = item->GetFirstChild();
	while(child){
		//child->SetActive(check); // seems this was implemented somewhere between ROOT 5.18 and 5.22
		child = child->GetNextSibling();
	}

#if 0
	if(check)
	{
		
/*		for(int c = 0; c < a.GetSize(); c++){
			CheckItem( dynamic_cast<TGListTreeItem *>(a-> Object) , Bool_t check = kTRUE);
			a-> Object = a-> Next;
			
		}
*/
		listTree->CheckAllChildren(item, check);
		_DBG_<<"Item should have checked all children true"<<check<<" for child of obj 0x"<<hex<< "-" <<(unsigned long)item<<dec<<endl;
		
	}
	else
	{

		listTree->CheckAllChildren(item, check);
		_DBG_<<"Item should have checked all children false "<<check<<" for child of obj 0x"<<hex<< "-" <<(unsigned long)item<<dec<<endl;
	}
#endif	
}


//---------------------------------
// DoSetViewByObject
//---------------------------------
/*
void Dialog_SaveHists::DoSetViewByObject(void)
{
	bool changed = (viewStyle != rs_info::kViewByObject);
	viewStyle = rs_info::kViewByObject;
	viewByObject->SetState(viewStyle==rs_info::kViewByObject ? kButtonDown:kButtonUp);
	viewByServer->SetState(viewStyle==rs_info::kViewByObject ? kButtonUp:kButtonDown);
	if(changed){
		vector<hid_t> hids;
		GetAllHistos(hids);
		UpdateListTree(hids);
	}
}

//---------------------------------
// DoSetViewByServer
//---------------------------------

void Dialog_SaveHists::DoSetViewByServer(void)
{
	bool changed = (viewStyle != rs_info::kViewByServer);
	viewStyle = rs_info::kViewByServer;
	viewByObject->SetState(viewStyle==rs_info::kViewByObject ? kButtonDown:kButtonUp);
	viewByServer->SetState(viewStyle==rs_info::kViewByObject ? kButtonUp:kButtonDown);
	if(changed){
		vector<hid_t> hids;
		GetAllHistos(hids);
		UpdateListTree(hids);
	}
}
*/
//---------------------------------
// GetAllHistos
//---------------------------------
void Dialog_SaveHists::GetAllHistos(vector<hid_t> &hids)
{
	RS_INFO->Lock();

	// Make list of all histograms from all servers
	map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.begin();
	for(; server_info_iter!=RS_INFO->servers.end(); server_info_iter++){//iterates over servers
		const string &server = server_info_iter->first;
		const vector<string> &hnamepaths = server_info_iter->second.hnamepaths;
		for(unsigned int j=0; j<hnamepaths.size(); j++){//iterates over histogram paths

			// add this hinfo_t object to the list
			hids.push_back(hid_t(server, hnamepaths[j]));
		}
	}
	RS_INFO->Unlock();
}

//---------------------------------
// UpdateListTree
//---------------------------------
void Dialog_SaveHists::UpdateListTree(vector<hid_t> hids)
{
	// Delete any existing items (seems like there should be a better way)
	TGListTreeItem *my_item = listTree->GetFirstItem();
	while(my_item){
		TGListTreeItem *next_item = my_item->GetNextSibling();
		listTree->DeleteItem(my_item);
		my_item = next_item;
	}
	server_items.clear();
	hist_items.clear();

	// Loop over histograms
	for(unsigned int i=0; i<hids.size(); i++){
		string &hnamepath = hids[i].hnamepath;
		string &server = hids[i].serverName;
		
		// Get hdef_t object for this hist
		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hids[i].hnamepath);
		if(hdef_iter==RS_INFO->histdefs.end()){
			_DBG_<<"No histo definition for hid: server=\""<<server<<"\" hnamepath=\""<<hnamepath<<"\"!"<<endl;
			continue;
		}
		hdef_t *hdef = &(hdef_iter->second);

		// Here we want to create a vector with each of the path
		// elements (folders) to be displayed and then the final
		// element (either histogram or server). 
		vector<string> path;
		string final;
		if(viewStyle==rs_info::kViewByObject){
			path = hdef->dirs;
			path.push_back(hdef->name);
			final = server;
		}else{
			path.push_back(server);
			path.insert(path.end(), hdef->dirs.begin(), hdef->dirs.end());
			final = hdef->name;
		}
		path.push_back(final);

		// Loop over path elements, adding them as needed
		vector<TGListTreeItem *> items;
		string mypath;
		for(unsigned int j=0; j<path.size(); j++){
			mypath += "/" + path[j];
			TGListTreeItem *item = listTree->FindItemByPathname(mypath.c_str());
			if(!item){
				TGListTreeItem *last_item = items.size()>0 ? items[items.size()-1]:NULL;
				item = listTree->AddItem(last_item, path[j].c_str());
			 	item->SetUserData(last_item);
				listTree->SetCheckBox(item, kTRUE);
				item->SetCheckBoxPictures(checked_t, unchecked_t);
				
				// Sets the directories to show all, with the 
				// exception of the servers if viewStyle is by object
				if(viewStyle==rs_info::kViewByObject && j == 0){
					item->SetOpen(true);
				} else if (viewStyle==rs_info::kViewByObject && j != 0){
					item->SetOpen(false);
				} else {
					item->SetOpen(true);
				}
			}
			items.push_back(item);
		}
		
		// Make sure we have at least 2 items (server and hist)
		if(items.size()<2)continue;
		
		// Set appropriate pictures and checkbox status for hist and 
		// server elements
		TGListTreeItem *server_item=NULL;
		TGListTreeItem *hist_item=NULL;
		bool server_checkbox=true;
		bool hist_checkbox=true;
		if(viewStyle==rs_info::kViewByObject){
			server_item = items[items.size()-1];
			hist_item = items[items.size()-2];
			server_checkbox = hdef->save_servers[server];
			hist_checkbox = hdef->active;
		}else{
			server_item = items[0];
			hist_item = items[items.size()-1];
			server_checkbox = RS_INFO->servers[server].active;
			hist_checkbox = hdef->save_servers[server];
		}
		server_item->SetPictures(hdisk_t, hdisk_t);
		hist_item->SetPictures(h1_t, h1_t);
		if(!server_checkbox)server_item->Toggle();
		if(!hist_checkbox)hist_item->Toggle();

		server_items[hid_t(server,hnamepath)] = server_item;
		hist_items[hid_t(server,hnamepath)] = hist_item;
	}
	
	// Remember the his list so we can easily check for changes
	last_hids = hids;
	
	// This is needed to force the list tree to draw its contents
	// without being clicked on. Without it (on at least one platform)
	// the window will simply appear blank until an invisible entry
	// is clicked on.
	listTree->ClearHighlighted();
}

//DoSave author: Justin B.
//Date: 06.21.10
// This method is called once the Save button is clicked. The Histograms are set active/inactive and
// then there is a request sent out for each active histogram. histtosave is a map that holds the paths of the
// histograms to be saved. The tosave boolean is set to true and that is recognized once the timer loops over it.
void Dialog_SaveHists::DoSave(void) {
	thefileinfo = new TGFileInfo();
	new TGFileDialog(gClient->GetRoot(), gClient->GetRoot(), kFDSave, thefileinfo);
//	_DBG__;
	displaybar = true;
	cout<<"display bar do save: "<<displaybar<<endl;
	// Call DoOK() to make sure the histograms are set active/inactive depending
	// on whether or not they are checked/unchecked.
	DoOK();
	RS_INFO->Lock();
	//by server
	map<string, hdef_t>::iterator histdefs_iter = RS_INFO->histdefs.begin();
	for(; histdefs_iter != RS_INFO->histdefs.end(); histdefs_iter++) {
		hdef_t hdefts = histdefs_iter->second;
		map<string, bool> active = hdefts.save_servers;
		map<string, bool>::iterator act_iter = active.begin();
		for(; act_iter != active.end(); act_iter++) {
			bool isactive = act_iter->second;
			if(isactive) {
				//numhist is a counter specifying
				//the number of histograms to be retrieved
				string server = act_iter->first;
				string path = hdefts.hnamepath;
				cout<<path<<endl;
				//add the active histograms to a map. Key is hnamepath. Map used for uniqueness.
				histtosave.insert(pair<string, int>(path, 0));
				//Send a request for the histograms to the cmsg server.
				if( RS_CMSG ) RS_CMSG->RequestHistogram(server, path);
				if( RS_XMSG ) RS_XMSG->RequestHistogram(server, path);
			}
		}
	}
	tosave = true;
	RS_INFO->Unlock();
}

//=====================
// AddDisplayBar
//=====================
void Dialog_SaveHists::AddDisplayBar(void) {
	// Add the load progress bar to the vertical frame on the GUI.
	progressbar = new TGHProgressBar(verticalframe,464);
	progressbar->SetFillType(TGProgressBar::kBlockFill);
	progressbar->ChangeOptions(kSunkenFrame | kDoubleBorder | kOwnBackground);

	ULong_t ucolor;        // will reflect user color changes
	gClient->GetColorByName("#ffffff",ucolor);
	progressbar->SetBackgroundColor(ucolor);
	progressbar->SetPosition(25);
	verticalframe->AddFrame(progressbar, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	mainframe->MapSubwindows();
	mainframe->Resize(mainframe->GetDefaultSize());
	mainframe->MapWindow();
	mainframe->Resize(490,372);
	displaybar = true;
}

//SearchListTree author: Justin B.
//Date: 06.23.10
//Recursive method that searches the TGListTree for checked items and places them in an array with
//their paths appended onto the names.

/*
void Dialog_SaveHists::SearchListTreeChecked(TGListTreeItem* current, vector<map<string, string> > &checked_path_names) {
		//if current does not have a parent
		if(current->IsChecked() && !current->GetParent()) {
			_DBG__;
			checked_path_names.push_back(map<string, string>());
		}
		//if current has no children
		if(current->IsChecked() && !current->GetFirstChild()){
			string current_name = current->GetText();
			string parent_name = current->GetParent()->GetText();
			string grandparent_name = current->GetParent()->GetParent()->GetText();
			string full_name = "/" + parent_name + "/" + current_name;
			checked_path_names[checked_path_names.size() - 1].insert(pair<string, string> (full_name,
												      grandparent_name));
		}
		//if current has a child get first child
		if(current->IsChecked() && current->GetFirstChild()) SearchListTreeChecked(current->GetFirstChild(),
												     checked_path_names);
		//get sibling
		if(current->GetNextSibling()) SearchListTreeChecked(current->GetNextSibling(), checked_path_names);
}
*/

//AreHistosPresent author: Justin B.
//Date: 06.21.10
/* This method is called once the timer recognizes the tosave boolean is true.
   This method checks to make sure that the histograms that were requested in
   the DoSave() method are all present. It simply mathces the histtosave map
   against the RS_INFO->histdef map.
   return: boolean

*/
bool Dialog_SaveHists::AreHistosPresent(void){
	RS_INFO->Lock();
	bool allreceived = true;
	// Iterate over the histtosave map and use its key as the input to
	// RS_INFO->histdefs.
	map<string, int>::iterator histtosave_iter = histtosave.begin();
	for(; histtosave_iter != histtosave.end(); histtosave_iter++) {
		hdef_t histodef = RS_INFO->histdefs[histtosave_iter->first];
		if(!histodef.sum_hist_present) allreceived = false;
	}
	if(allreceived) {
		cerr<<"The Histograms requested have been returned."<<endl;
		//cerr<<"true " <<_DBG__;
		RS_INFO->Unlock();
		return true;
	}
	cerr<<"All of the Histograms requested have not yet returned."<<endl;
	RS_INFO->Unlock();
	//cerr<<"false " <<_DBG__;
	return false;
}

//SaveHistograms author: Justin B.
//Date: 06.21.10
// Method to save histograms as .root files.
void Dialog_SaveHists::SaveHistograms(void){
	RS_INFO->Lock();
	// Create a directory to write to.
	TDirectory* save_dir = gDirectory;
	TFile* outfile = new TFile(thefileinfo->fFilename, "RECREATE");
	outfile->cd();

	map<string, int>::iterator histtosave_iter = histtosave.begin();
	for(; histtosave_iter != histtosave.end(); histtosave_iter++) {
		hdef_t histodef = RS_INFO->histdefs[histtosave_iter->first];
		TH1* sumhistSave = histodef.sum_hist;
		_DBG__;
		sumhistSave->Write();
	}
	displaybar = false;
	progressbar->Reset();
	delete thefileinfo;
	tosave = false;
	histtosave.clear();
	outfile->Close();
	save_dir->cd();
	RS_INFO->Unlock();
}

//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_SaveHists::CreateGUI(void)
{
	  // main frame
      TGMainFrame *fMainFrame2997 = this;
      fMainFrame2997->SetLayoutBroken(kTRUE);

      // composite frame
      TGCompositeFrame *fMainFrame2829 = new TGCompositeFrame(fMainFrame2997,490,372,kVerticalFrame);
      fMainFrame2829->SetLayoutBroken(kTRUE);

      // composite frame
      TGCompositeFrame *fCompositeFrame2830 = new TGCompositeFrame(fMainFrame2829,490,372,kVerticalFrame);
      fCompositeFrame2830->SetLayoutBroken(kTRUE);

      // composite frame
      TGCompositeFrame *fCompositeFrame2831 = new TGCompositeFrame(fCompositeFrame2830,490,372,kVerticalFrame);
      fCompositeFrame2831->SetLayoutBroken(kTRUE);

      // composite frame
      TGCompositeFrame *fCompositeFrame2832 = new TGCompositeFrame(fCompositeFrame2831,488,370,kVerticalFrame);

      // vertical frame
      TGVerticalFrame *fVerticalFrame2833 = new TGVerticalFrame(fCompositeFrame2832,484,366,kVerticalFrame);

      // vertical frame
      TGVerticalFrame *fVerticalFrame2834 = new TGVerticalFrame(fVerticalFrame2833,387,65,kVerticalFrame);
      TGLabel *fLabel2835 = new TGLabel(fVerticalFrame2834,"To save a sum histogram you must place a check\n in a box located under the histogram name. This check corresponds\n to which servers you will include in the sum\nhistogram .root file, if any.");
      fLabel2835->SetTextJustify(36);
      fLabel2835->SetMargins(0,0,0,0);
      fLabel2835->SetWrapLength(-1);
      fVerticalFrame2834->AddFrame(fLabel2835, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

      fVerticalFrame2833->AddFrame(fVerticalFrame2834, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

      // canvas widget
      TGCanvas *fCanvas2836 = new TGCanvas(fVerticalFrame2833,360,192);

      // canvas viewport
      TGViewPort *fViewPort2837 = fCanvas2836->GetViewPort();

      // list tree
      TGListTree *fListTree2846 = new TGListTree(fCanvas2836,kHorizontalFrame);

      const TGPicture *popen;       //used for list tree items
      const TGPicture *pclose;      //used for list tree items

      TGListTreeItem *item0 = fListTree2846->AddItem(NULL,"Entry 1");
      popen = gClient->GetPicture("ofolder_t.xpm");
      pclose = gClient->GetPicture("folder_t.xpm");
      item0->SetPictures(popen, pclose);
      fListTree2846->CloseItem(item0);
      fListTree2846->SetSelected(item0);

      fViewPort2837->AddFrame(fListTree2846);
      fListTree2846->SetLayoutManager(new TGHorizontalLayout(fListTree2846));
      fListTree2846->MapSubwindows();
      fCanvas2836->SetContainer(fListTree2846);
      fCanvas2836->MapSubwindows();
      fVerticalFrame2833->AddFrame(fCanvas2836, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

      // horizontal frame
      TGHorizontalFrame *fHorizontalFrame2847 = new TGHorizontalFrame(fVerticalFrame2833,400,32,kHorizontalFrame);
      fHorizontalFrame2847->SetLayoutBroken(kTRUE);
      TGTextButton *fTextButton2848 = new TGTextButton(fHorizontalFrame2847,"Cancel");
      fTextButton2848->SetTextJustify(36);
      fTextButton2848->SetMargins(0,0,0,0);
      fTextButton2848->SetWrapLength(-1);
      fTextButton2848->Resize(48,22);
      fHorizontalFrame2847->AddFrame(fTextButton2848, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
      fTextButton2848->MoveResize(2,2,48,22);
      TGTextButton *fTextButton2849 = new TGTextButton(fHorizontalFrame2847,"Save");
      fTextButton2849->SetTextJustify(36);
      fTextButton2849->SetMargins(0,0,0,0);
      fTextButton2849->SetWrapLength(-1);
      fTextButton2849->Resize(37,22);
      fHorizontalFrame2847->AddFrame(fTextButton2849, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
      fTextButton2849->MoveResize(54,2,37,22);

      fVerticalFrame2833->AddFrame(fHorizontalFrame2847, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

      //Progress Bar
      //===============================================================================================================
      TGHProgressBar *fHProgressBar2866 = new TGHProgressBar(fVerticalFrame2833,464);
      fHProgressBar2866->SetFillType(TGProgressBar::kSolidFill);
      fHProgressBar2866->ChangeOptions(kSunkenFrame | kDoubleBorder | kOwnBackground);

      ULong_t ucolor;        // will reflect user color changes
      gClient->GetColorByName("#ffffff",ucolor);
      fHProgressBar2866->SetBackgroundColor(ucolor);
      fHProgressBar2866->SetPosition(25);
      fVerticalFrame2833->AddFrame(fHProgressBar2866, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
      fHProgressBar2866->Reset();

      //===============================================================================================================
      fCompositeFrame2832->AddFrame(fVerticalFrame2833, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

      fCompositeFrame2831->AddFrame(fCompositeFrame2832, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      fCompositeFrame2832->MoveResize(0,0,488,370);

      fCompositeFrame2830->AddFrame(fCompositeFrame2831, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      fCompositeFrame2831->MoveResize(0,0,490,372);

      fMainFrame2829->AddFrame(fCompositeFrame2830, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      fCompositeFrame2830->MoveResize(0,0,490,372);

      fMainFrame2997->AddFrame(fMainFrame2829, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      fMainFrame2829->MoveResize(0,0,490,372);

      fMainFrame2997->SetMWMHints(kMWMDecorAll,
                           kMWMFuncAll,
                           kMWMInputModeless);
      fMainFrame2997->MapSubwindows();

      fMainFrame2997->Resize(fMainFrame2997->GetDefaultSize());
      fMainFrame2997->MapWindow();
      fMainFrame2997->Resize(490,372);

    //===============================================================================================================

	// Connect GUI elements to methods
	TGTextButton* &save = fTextButton2849;
	TGTextButton* &cancel = fTextButton2848;
	this->canvas = fCanvas2836;
	this->listTree = fListTree2846;
	this->progressbar = fHProgressBar2866;
//	this->verticalframe = fVerticalFrame2833;
//	mainframe = fMainFrame2997;

	save->Connect("Clicked()","Dialog_SaveHists",this, "DoSave()");
	cancel->Connect("Clicked()","Dialog_SaveHists", this, "CloseWindow()");
	this->listTree->Connect("Clicked(TGListTreeItem*, Int_t)","Dialog_SaveHists", this, "DoClickedEntry (TGListTreeItem*, Int_t)");
	this->listTree->Connect("Checked(TObject*, Bool_t)","Dialog_SaveHists", this, "DoCheckedEntry(TObject*, Int_t)");

	folder_t = pclose;
	ofolder_t = popen;
	h1_s = gClient->GetPicture("h1_s.xpm");
	h1_t = gClient->GetPicture("h1_t.xpm");
	h2_s = gClient->GetPicture("h2_s.xpm");
	h2_t = gClient->GetPicture("h2_t.xpm");
	h3_s = gClient->GetPicture("h3_s.xpm");
	h3_t = gClient->GetPicture("h3_t.xpm");
	pack_t = gClient->GetPicture("pack_t.xpm");
	hdisk_t = gClient->GetPicture("hdisk_t.xpm");
	checked_t = gClient->GetPicture("checked_t.xpm");
	unchecked_t = gClient->GetPicture("unchecked_t.xpm");
	
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

