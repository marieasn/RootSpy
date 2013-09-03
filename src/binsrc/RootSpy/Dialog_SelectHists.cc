// $Id$
//
//    File: Dialog_SelectHists.cc
// Created: Mon Sep 14 08:54:47 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_SelectHists.h"
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
#include <unistd.h>
#include <TGFileDialog.h>

//---------------------------------
// Dialog_SelectHists    (Constructor)
//---------------------------------
Dialog_SelectHists::Dialog_SelectHists(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	// Define all of the of the graphics objects. 
	CreateGUI();
	cout<<gClient->GetPicturePool()->GetPath()<<endl;
	// Set up timer to call the DoTimer() method repeatedly
	// so events can be automatically advanced.
	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_SelectHists", this, "DoTimer()");
	sleep_time = 500;
	timer->Start(sleep_time, kFALSE);

	time_t now = time(NULL) - 3;
	last_called = now;
	last_ping_time = now;
	last_hist_time = now;
	
	//viewStyle = rs_info::kViewByServer;
	viewStyle = rs_info::kViewByObject;
	
	DoTimer();

	// Finish up and map the window
	SetWindowName("RootSpy Select Server/Histogram");
	SetIconName("SelectHist");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//---------------------------------
// ~Dialog_SelectHists    (Destructor)
//---------------------------------
Dialog_SelectHists::~Dialog_SelectHists()
{

	cout<<"escaped destructor"<<endl;
}


void Dialog_SelectHists::CloseWindow(void) {
	timer->Stop();
	delete timer;
	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_selecthists = true;
}
//-------------------
// DoTimer
//-------------------
void Dialog_SelectHists::DoTimer(void)
{
	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	
	time_t now = time(NULL);

	// Ping servers occasionally to make sure our server list is up-to-date
	if(now-last_ping_time >= 3){
		RS_CMSG->PingServers();
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
				RS_CMSG->RequestHists(servername);
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
}

//---------------------------------
// DoOK
//---------------------------------
void Dialog_SelectHists::DoOK(void)
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
			hdef_iter->second.servers[hid.serverName] = item->IsChecked();
		}else{
			map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(hid.serverName);
			server_info_iter->second.active = item->IsChecked();
		}
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
			hdef_iter->second.active = item->IsChecked();
		}else{
			hdef_iter->second.servers[hid.serverName] = item->IsChecked();
		}
	}

	// If the RS_INFO->current value is not set, then set it to the first server/histo
	// and set the flag to have DoUpdate called
	if(RS_INFO->servers.find(RS_INFO->current.serverName)==RS_INFO->servers.end()){
		if(RS_INFO->servers.size()>0){
			map<string,server_info_t>::iterator server_iter = RS_INFO->servers.begin();
			if(server_iter->second.hnamepaths.size()>0){
				RS_INFO->current = hid_t(server_iter->first, server_iter->second.hnamepaths[0]);
			}
		}
	}
	RSMF->can_view_indiv = true;
	RS_INFO->viewStyle = viewStyle;
	RS_INFO->update = true;


	RS_INFO->Unlock();
	
	
	DoCancel();
}

//---------------------------------
// DoCancel
//---------------------------------
void Dialog_SelectHists::DoCancel(void)
{
	timer->Stop();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_selecthists = true;
}

//---------------------------------
// DoClickedEntry
//---------------------------------
void Dialog_SelectHists::DoClickedEntry(TGListTreeItem* entry, Int_t btn)
{
	// Called whenever an item is selected in listTree

	char path[512];
	listTree->GetPathnameFromItem(entry, path);
	_DBG_<<"Clicked entry \""<<path<<"\" with btn="<<btn<<endl;
}

//---------------------------------
// DoCheckedEntry
//---------------------------------
void Dialog_SelectHists::DoCheckedEntry(TObject* obj, Int_t check)
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
void Dialog_SelectHists::DoSetViewByObject(void)
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
void Dialog_SelectHists::DoSetViewByServer(void)
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

//---------------------------------
// GetAllHistos
//---------------------------------
void Dialog_SelectHists::GetAllHistos(vector<hid_t> &hids)
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
void Dialog_SelectHists::UpdateListTree(vector<hid_t> hids)
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
			server_checkbox = hdef->servers[server];
			hist_checkbox = hdef->active;
		}else{
			server_item = items[0];
			hist_item = items[items.size()-1];
			server_checkbox = RS_INFO->servers[server].active;
			hist_checkbox = hdef->servers[server];
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


//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_SelectHists::CreateGUI(void)
{
	TGMainFrame *fMainFrame984 = this;

   // composite frame
   TGCompositeFrame *fMainFrame800 = new TGCompositeFrame(fMainFrame984,400,290,kVerticalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame801 = new TGVerticalFrame(fMainFrame800,396,286,kVerticalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame802 = new TGVerticalFrame(fVerticalFrame801,96,42,kVerticalFrame);
   TGRadioButton *fRadioButton803 = new TGRadioButton(fVerticalFrame802,"View By Object");
   fRadioButton803->SetState(kButtonDown);
   //fRadioButton803->SetState(kButtonUp);
   fRadioButton803->SetTextJustify(36);
   fRadioButton803->SetMargins(0,0,0,0);
   fRadioButton803->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton803, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGRadioButton *fRadioButton804 = new TGRadioButton(fVerticalFrame802,"View By Server");
   //fRadioButton804->SetState(kButtonDown);
   fRadioButton804->SetState(kButtonUp);
   fRadioButton804->SetTextJustify(36);
   fRadioButton804->SetMargins(0,0,0,0);
   fRadioButton804->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton804, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fVerticalFrame801->AddFrame(fVerticalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // canvas widget
   TGCanvas *fCanvas805 = new TGCanvas(fVerticalFrame801,384,200);

   // canvas viewport
   TGViewPort *fViewPort806 = fCanvas805->GetViewPort();

   // list tree
   TGListTree *fListTree815 = new TGListTree(fCanvas805,kHorizontalFrame);

   const TGPicture *popen;       //used for list tree items
   const TGPicture *pclose;      //used for list tree items

   //TGListTreeItem *item0 = fListTree815->AddItem(NULL,"Entry 1");
   popen = gClient->GetPicture("ofolder_t.xpm");
   pclose = gClient->GetPicture("folder_t.xpm");
   //item0->SetPictures(popen, pclose);
   //fListTree815->CloseItem(item0);

   fViewPort806->AddFrame(fListTree815);
   fListTree815->SetLayoutManager(new TGHorizontalLayout(fListTree815));
   fListTree815->MapSubwindows();
   fCanvas805->SetContainer(fListTree815);
   fCanvas805->MapSubwindows();
   fVerticalFrame801->AddFrame(fCanvas805, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame816 = new TGHorizontalFrame(fVerticalFrame801,392,32,kHorizontalFrame);
   fHorizontalFrame816->SetLayoutBroken(kTRUE);
   TGTextButton *fTextButton817 = new TGTextButton(fHorizontalFrame816,"Cancel");
   fTextButton817->SetTextJustify(36);
   fTextButton817->SetMargins(0,0,0,0);
   fTextButton817->SetWrapLength(-1);
   fTextButton817->Resize(56,22);
   fHorizontalFrame816->AddFrame(fTextButton817, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton817->MoveResize(2,2,56,22);
   TGTextButton *fTextButton818 = new TGTextButton(fHorizontalFrame816,"OK");
   fTextButton818->SetTextJustify(36);
   fTextButton818->SetMargins(0,0,0,0);
   fTextButton818->SetWrapLength(-1);
   fTextButton818->Resize(55,22);
   fHorizontalFrame816->AddFrame(fTextButton818, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton818->MoveResize(320,0,55,22);

   fVerticalFrame801->AddFrame(fHorizontalFrame816, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fMainFrame800->AddFrame(fVerticalFrame801, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fMainFrame984->AddFrame(fMainFrame800, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fMainFrame800->MoveResize(0,0,400,290);

   fMainFrame984->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame984->MapSubwindows();

   fMainFrame984->Resize(fMainFrame984->GetDefaultSize());
   fMainFrame984->MapWindow();
   fMainFrame984->Resize(490,372);
	//==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &ok = fTextButton818;
	TGTextButton* &cancel = fTextButton817;
	this->canvas = fCanvas805;
	this->listTree = fListTree815;
	this->viewByObject = fRadioButton803;
	this->viewByServer = fRadioButton804;
	
	ok->Connect("Clicked()","Dialog_SelectHists", this, "DoOK()");
	cancel->Connect("Clicked()","Dialog_SelectHists", this, "DoCancel()");
	this->listTree->Connect("Clicked(TGListTreeItem*, Int_t)","Dialog_SelectHists", this, "DoClickedEntry(TGListTreeItem*, Int_t)");
	this->listTree->Connect("Checked(TObject*, Bool_t)","Dialog_SelectHists", this, "DoCheckedEntry(TObject*, Int_t)");
	this->viewByObject->Connect("Clicked()","Dialog_SelectHists", this, "DoSetViewByObject()");
	this->viewByServer->Connect("Clicked()","Dialog_SelectHists", this, "DoSetViewByServer()");
	//Connect("CloseWindow()", "Dialog_SelectHists", this, "DoCancel()");	
	
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

