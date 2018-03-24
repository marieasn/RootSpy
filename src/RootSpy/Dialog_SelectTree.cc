
#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_SelectTree.h"
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
#include <TG3DLine.h>

//---------------------------------
// Dialog_SelectTree    (Constructor)
//---------------------------------
Dialog_SelectTree::Dialog_SelectTree(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame) {
	// Define all of the of the graphics objects.
	CreateGUI();

	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_SelectTree", this, "DoTimer()");
	sleep_time = 500;
	timer->Start(sleep_time, kFALSE);
	time_t now = time(NULL) - 3;
	last_called = now;
	last_ping_time = now;
	last_hist_time = now;
	requested = false;
	lastsize = 0;
	DoTimer();

	// Finish up and map the window
	SetWindowName("RootSpy Select Tree");
	SetIconName("SelectTree");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//---------------------------------
// ~Dialog_SelectTree    (Destructor)
//---------------------------------
Dialog_SelectTree::~Dialog_SelectTree() {

}


void Dialog_SelectTree::CloseWindow(void) {
	timer->Stop();
	delete timer;
	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_selecttree = true;
}

//-------------------
// DoTimer
//-------------------
void Dialog_SelectTree::DoTimer(void) {

	/// This gets called periodically (value is set in constructor)
	/// It's main job is to communicate with the callback through
	/// data members more or less asynchronously.
	time_t now = time(NULL);

	// Ping servers occasionally to make sure our list is up-to-date
	if(now-last_ping_time >= 3) {
		if( RS_CMSG) RS_CMSG->PingServers();
		if( RS_XMSG) RS_XMSG->PingServers();
		last_ping_time = now;
	}

	// Ping server occasionally to make sure our tree list is up-to-date
	if(now-last_hist_time >= 3) {
		// Loop over servers
		if(!requested) {
			RS_INFO->Lock ();
			map<string,server_info_t>::iterator iter = RS_INFO->servers.begin();
			for(; iter!=RS_INFO->servers.end(); iter++) {
				string servername = iter->first;
				if(servername != "") {
					if( RS_CMSG) RS_CMSG->RequestTreeInfo(servername);
				}
			}
			RS_INFO->Unlock();
			last_hist_time = now;
			requested = true;
		}
	}
	vector<vector<tree_info_t> > tree_ids;
	getAllTrees(tree_ids);
	//Check if list of trees has changed
	//TODO: may have to change the method by which a change
	//in the list trees is detected. Currently, the change is
	//detected by a change in the tree_ids size. Maybe there
	//is branch information that changes.
	if(lastsize != tree_ids.size()) {
		updateListTreeServer(RS_INFO->servers);
//		updateListTree(tree_ids);
	}
	lastsize = tree_ids.size();
	last_called = now;
}

//---------------------------------
// getAllTrees
//---------------------------------
//TODO: documentation comment.
void Dialog_SelectTree::getAllTrees(vector<vector<tree_info_t> > &tree_ids) {
	RS_INFO->Lock();
	map<string, server_info_t>::iterator server_iter = RS_INFO->servers.begin();
	for(; server_iter != RS_INFO->servers.end(); server_iter++) {
		tree_ids.push_back(server_iter->second.trees);
//		cout<<tree_ids.size()<<endl;
//		for(unsigned int i = 0; i< server_iter->second.trees.size(); i++) {
//			_DBG__;
//			cout<<server_iter->second.trees[i].path<<endl;
//			cout<<server_iter->second.trees[i].name<<endl;
////			cout<<server_iter->second.trees[i].branch_info<<endl;
//		}
	}
	RS_INFO->Unlock();
}

//---------------------------------
// updateListTreeServer
//---------------------------------
//TODO: documentation comment.
void Dialog_SelectTree::updateListTreeServer(map<string, server_info_t> &servinfo) {
	// Delete any existing items (seems like there should be a better way)
	TGListTreeItem *my_item = list_tree->GetFirstItem();
	while(my_item){
		TGListTreeItem *next_item = my_item->GetNextSibling();
		list_tree->DeleteItem(my_item);
		my_item = next_item;
	}
	server_items.clear();
	tree_items.clear();

	//TODO: When > 1 tree servers are running, the list tree seems to display
	// too many trees under one folder.
	map<string, server_info_t>::iterator server_iter = servinfo.begin();
	for(; server_iter != RS_INFO->servers.end(); server_iter++) {
		cerr<<"firstLoop"<<endl;
		const char* servername = server_iter->first.c_str();
		TGListTreeItem *serveritem = list_tree->AddItem(NULL, servername);
		serveritem->SetPictures(hdisk_t, hdisk_t);
		vector<tree_info_t> treeinfo = server_iter->second.trees;
		vector<tree_info_t>::iterator veciter = treeinfo.begin();
		const char* pathname = veciter->path.c_str();
		TGListTreeItem *pathitem = list_tree->AddItem(serveritem, pathname);
		for(; veciter != treeinfo.end(); veciter++) {
			cerr<<"secondLoop"<<endl;
			const char* name = veciter->name.c_str();
			TGListTreeItem *nameitem = list_tree->AddItem(pathitem, name);
			nameitem->SetPictures(tree_s, tree_t);
		}
	}
}

//---------------------------------
// updateListTree
//---------------------------------
//TODO: documentation comment.
void Dialog_SelectTree::updateListTree(vector<vector<tree_info_t> > &tree_ids) {
	// Delete any existing items (seems like there should be a better way)
	TGListTreeItem *my_item = list_tree->GetFirstItem();
	while(my_item){
		TGListTreeItem *next_item = my_item->GetNextSibling();
		list_tree->DeleteItem(my_item);
		my_item = next_item;
	}
	server_items.clear();
	tree_items.clear();

//	string thrown = "thrown";
//	TGListTreeItem *item = list_tree->AddItem(NULL, thrown.c_str());
//	item->SetPictures(hdisk_t, hdisk_t);
//	TGListTreeItem *item2 = list_tree->AddItem(item, "child");
//	item2->SetPictures(hdisk_t, hdisk_t);
//	string thrown2 = "recon";
//	TGListTreeItem *item3 = list_tree->AddItem(NULL, thrown2.c_str());
//	item->SetPictures(hdisk_t, hdisk_t);
//	TGListTreeItem *item4 = list_tree->AddItem(item3, "child");
//	item2->SetPictures(hdisk_t, hdisk_t);

	//TODO: for(int... instead of the second iterator.
	vector<vector<tree_info_t> >::iterator vec_iter = tree_ids.begin();
	for(; vec_iter != tree_ids.end(); vec_iter++) {
		vector<tree_info_t> tree = *vec_iter;
		vector<tree_info_t>::iterator tree_iter = tree.begin();
		const char* servername = tree_iter->servername.c_str();
		list_tree->AddItem(NULL, servername);
		for(; tree_iter != tree.end(); tree_iter++) {
//			cout<<tree_iter->path<<endl;
		}
	}

//	vector<vector<tree_info_t> >::iterator vec_iter = tree_ids.begin();
//	for(; vec_iter != tree_ids.end(); vec_iter++) {
//		vector<tree_info_t> tree = *vec_iter;
//		for(int i = 0; i<tree.size(); i++) {
//			tree[]
//		}
//	}

//	tree_info_t inf = (tree_info_t) tree_ids[0][0];
//	string path = inf.path;
//	cout<<path<<endl;

//	for (int i = 0; i < tree_ids.size(); i++) {
//		cerr<<i<<endl;
//		for(int j = 0; j < tree_ids[i].size(); j ++) {
//			cout<<j<<endl;
//			cout<<tree_ids[i][j].path<<endl;
//			cout<<tree_ids[i][j].name<<endl;
//		}
//	}

//	TGListTreeItem *item = list_tree->AddItem(NULL, tree_ids[0][0].serverName.c_str());
//	item->SetPictures(hdisk_t, hdisk_t);
//	TGListTreeItem *item2 = list_tree->AddItem(item, tree_ids[0][0].path.c_str());
//	item2->SetPictures(folder_t, folder_t);

//	for(unsigned int i = 0; i < tree_ids.size(); i++) {
//		const char* server_name = tree_ids[i][0].serverName.c_str();
//		TGListTreeItem *server_item = list_tree->AddItem(NULL, server_name);
//		const char* folder_name = tree_ids[i][0].path.c_str();
////		cout<<i<< " folder name: "<<tree_ids[i][0].path<<endl;
////		cout<<"folder name: "<<folder_name<<endl;
//		TGListTreeItem *folder_item = list_tree->AddItem(server_item, folder_name);
//		for(unsigned int j = 0; j < tree_ids[i].size(); j++) {
////			cout<<tree_ids[i][j].name<<endl;
////			const char* tree_name = tree_ids[i][j].name.c_str();
////			TGListTreeItem *tree_item = list_tree->AddItem(folder_item, tree_name);
//		}
//	}

//	// Loop over trees
//	for(unsigned int i=0; i<hids.size(); i++) {
//		string &hnamepath = hids[i].hnamepath;
//		string &server = hids[i].serverName;
//
//		// Get hdef_t object for this hist
//		map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hids[i].hnamepath);
//		if(hdef_iter==RS_INFO->histdefs.end()){
//			_DBG_<<"No histo definition for hid: server=\""<<server<<"\" hnamepath=\""<<hnamepath<<"\"!"<<endl;
//			continue;
//		}
//		hdef_t *hdef = &(hdef_iter->second);
//
//		// Here we want to create a vector with each of the path
//		// elements (folders) to be displayed and then the final
//		// element (either histogram or server).
//		vector<string> path;
//		string final;
//		if(viewStyle==rs_info::kViewByObject){
//			path = hdef->dirs;
//			path.push_back(hdef->name);
//			final = server;
//		}else{
//			path.push_back(server);
//			path.insert(path.end(), hdef->dirs.begin(), hdef->dirs.end());
//			final = hdef->name;
//		}
//		path.push_back(final);
//
//		// Loop over path elements, adding them as needed
//		vector<TGListTreeItem *> items;
//		string mypath;
//		for(unsigned int j=0; j<path.size(); j++){
//			mypath += "/" + path[j];
//			TGListTreeItem *item = listTree->FindItemByPathname(mypath.c_str());
//			if(!item){
//				TGListTreeItem *last_item = items.size()>0 ? items[items.size()-1]:NULL;
//				item = listTree->AddItem(last_item, path[j].c_str());
//			 	item->SetUserData(last_item);
//				listTree->SetCheckBox(item, kTRUE);
//				item->SetCheckBoxPictures(checked_t, unchecked_t);
//
//				// Sets the directories to show all, with the
//				// exception of the servers if viewStyle is by object
//				if(viewStyle==rs_info::kViewByObject && j == 0){
//					item->SetOpen(true);
//				} else if (viewStyle==rs_info::kViewByObject && j != 0){
//					item->SetOpen(false);
//				} else {
//					item->SetOpen(true);
//				}
//			}
//			items.push_back(item);
//		}
//
//		// Make sure we have at least 2 items (server and hist)
//		if(items.size()<2)continue;
//
//		// Set appropriate pictures and checkbox status for hist and
//		// server elements
//		TGListTreeItem *server_item=NULL;
//		TGListTreeItem *hist_item=NULL;
//		bool server_checkbox=true;
//		bool hist_checkbox=true;
//		if(viewStyle==rs_info::kViewByObject){
//			server_item = items[items.size()-1];
//			hist_item = items[items.size()-2];
//			server_checkbox = hdef->servers[server];
//			hist_checkbox = hdef->active;
//		}else{
//			server_item = items[0];
//			hist_item = items[items.size()-1];
//			server_checkbox = RS_INFO->servers[server].active;
//			hist_checkbox = hdef->servers[server];
//		}
//		server_item->SetPictures(hdisk_t, hdisk_t);
//		hist_item->SetPictures(h1_t, h1_t);
//		if(!server_checkbox)server_item->Toggle();
//		if(!hist_checkbox)hist_item->Toggle();
//
//		server_items[hid_t(server,hnamepath)] = server_item;
//		hist_items[hid_t(server,hnamepath)] = hist_item;
//	}
//
//	// Remember the his list so we can easily check for changes
//	last_hids = hids;
//
//	// This is needed to force the list tree to draw its contents
//	// without being clicked on. Without it (on at least one platform)
//	// the window will simply appear blank until an invisible entry
//	// is clicked on.
//	listTree->ClearHighlighted();
}

//---------------------------------
// DoDoubleClickedEntry
//---------------------------------
void Dialog_SelectTree::DoDoubleClickedEntry(TGListTreeItem* entry, Int_t btn) {

}

//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_SelectTree::Dialog_SelectTree::CreateGUI(void) {
	// main frame
	TGMainFrame *fMainFrame1211 = this;
	fMainFrame1211->SetLayoutBroken(kTRUE);

	// horizontal frame
	TGHorizontalFrame *fHorizontalFrame560 = new TGHorizontalFrame(fMainFrame1211,635,376,kHorizontalFrame);
	fHorizontalFrame560->SetLayoutBroken(kTRUE);

	// canvas widget
	TGCanvas *fCanvas758 = new TGCanvas(fHorizontalFrame560,631,372);

	// canvas viewport
	TGViewPort *fViewPort759 = fCanvas758->GetViewPort();

	// list tree
	TGListTree *fListTree768 = new TGListTree(fCanvas758,kHorizontalFrame);

	const TGPicture *popen;       //used for list tree items
	const TGPicture *pclose;      //used for list tree items

	popen = gClient->GetPicture("ofolder_t.xpm");
    pclose = gClient->GetPicture("folder_t.xpm");

//	TGListTreeItem *item5 = fListTree768->AddItem(NULL,"Entry 1");
//	item5->SetPictures(popen, pclose);
//	fListTree768->OpenItem(item5);
//	fListTree768->SetSelected(item5);
//	TGListTreeItem *item6 = fListTree768->AddItem(NULL,"Entry 2");
//	item6->SetPictures(popen, pclose);
//	fListTree768->CloseItem(item6);
//	TGListTreeItem *item7 = fListTree768->AddItem(NULL,"Entry 3");
//	item7->SetPictures(popen, pclose);
//	fListTree768->CloseItem(item7);
//	TGListTreeItem *item8 = fListTree768->AddItem(NULL,"Entry 4");
//	item8->SetPictures(popen, pclose);
//	fListTree768->CloseItem(item8);
//	TGListTreeItem *item9 = fListTree768->AddItem(NULL,"Entry 5");
//	item9->SetPictures(popen, pclose);
//	fListTree768->CloseItem(item9);

	fViewPort759->AddFrame(fListTree768);
	fListTree768->SetLayoutManager(new TGHorizontalLayout(fListTree768));
	fListTree768->MapSubwindows();
	fCanvas758->SetContainer(fListTree768);
	fCanvas758->MapSubwindows();
	fHorizontalFrame560->AddFrame(fCanvas758, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fCanvas758->MoveResize(2,2,631,372);

	fMainFrame1211->AddFrame(fHorizontalFrame560, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontalFrame560->MoveResize(2,82,635,376);

	// horizontal frame
	TGHorizontalFrame *fHorizontalFrame575 = new TGHorizontalFrame(fMainFrame1211,635,56,kHorizontalFrame);
	fHorizontalFrame575->SetLayoutBroken(kTRUE);
	TGLabel *fLabel621 = new TGLabel(fHorizontalFrame575,"Please double click on the tree you would like to see information from.");
	fLabel621->SetTextJustify(36);
	fLabel621->SetMargins(0,0,0,0);
	fLabel621->SetWrapLength(-1);
	fHorizontalFrame575->AddFrame(fLabel621, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fLabel621->MoveResize(2,2,630,52);

	fMainFrame1211->AddFrame(fHorizontalFrame575, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontalFrame575->MoveResize(2,2,635,56);
	TGHorizontal3DLine *fHorizontal3DLine584 = new TGHorizontal3DLine(fMainFrame1211,635,8);
	fMainFrame1211->AddFrame(fHorizontal3DLine584, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontal3DLine584->MoveResize(0,72,635,8);

	// horizontal frame
	TGHorizontalFrame *fHorizontalFrame670 = new TGHorizontalFrame(fMainFrame1211,635,71,kHorizontalFrame);
	fHorizontalFrame670->SetLayoutBroken(kTRUE);
	TGTextButton *fTextButton702 = new TGTextButton(fHorizontalFrame670,"Cancel");
	fTextButton702->SetTextJustify(36);
	fTextButton702->SetMargins(0,0,0,0);
	fTextButton702->SetWrapLength(-1);
	fTextButton702->Resize(92,24);
	fHorizontalFrame670->AddFrame(fTextButton702, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fTextButton702->MoveResize(528,24,92,24);

	fMainFrame1211->AddFrame(fHorizontalFrame670, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontalFrame670->MoveResize(2,472,635,71);
	TGHorizontal3DLine *fHorizontal3DLine677 = new TGHorizontal3DLine(fMainFrame1211,635,8);
	fMainFrame1211->AddFrame(fHorizontal3DLine677, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
	fHorizontal3DLine677->MoveResize(0,464,635,8);

	fMainFrame1211->SetMWMHints(kMWMDecorAll,
						kMWMFuncAll,
						kMWMInputModeless);
	fMainFrame1211->MapSubwindows();

	fMainFrame1211->Resize(fMainFrame1211->GetDefaultSize());
	fMainFrame1211->MapWindow();
	fMainFrame1211->Resize(639,545);

	//==============================================================================================

	cancel = fTextButton702;
	cancel->Connect("Clicked()","Dialog_SelectTree", this, "CloseWindow()");

	list_tree = fListTree768;
	list_tree->Connect("DoubleClicked(TGListTreeItem*, Int_t)","Dialog_SelectTree",
			this, "DoDoubleClickedEntry(TGListTreeItem*, Int_t)");

	folder_t = pclose;
	ofolder_t = popen;
	tree_s = gClient->GetPicture("tree_s.xpm");
	tree_t = gClient->GetPicture("tree_t.xpm");
	pack_t = gClient->GetPicture("pack_t.xpm");
	hdisk_t = gClient->GetPicture("hdisk_t.xpm");
}

