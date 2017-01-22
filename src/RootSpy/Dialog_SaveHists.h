// $Id$
//
//    File: Dialog_SaveHists.h
// Created: 06.21.10
// Creator: Justin B (I just copied the source code from Davidl's Dialog_SelectHists.h and added some things).
//

#ifndef _Dialog_SaveHists_
#define _Dialog_SaveHists_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListTree.h>
#include <TGFileDialog.h>
#include <TGPicture.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <TGProgressBar.h>
//#include<TGWindow.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "../libRootSpy-client/hinfo_t.h"

using namespace std;

class Dialog_SaveHists:public TGMainFrame{
	public:
		Dialog_SaveHists(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_SaveHists();
		
		
		void DoTimer(void);
		void DoOK(void);
		void DoCancel(void);
		void DoClickedEntry(TGListTreeItem* entry, Int_t btn);
		void DoCheckedEntry(TObject* obj, Int_t check);
		//void DoSetViewByObject(void);
		//void DoSetViewByServer(void);
		void DoSave(void);
		void AddDisplayBar(void);
		void CloseWindow(void);
		void SaveHistograms(void);
		bool AreHistosPresent(void);
		//void SearchListTreeChecked(TGListTreeItem* current, vector<vector<string> > &checked_path_names);
		//void SearchListTreeChecked(TGListTreeItem* current, vector<map<string, string> >&checked_path_names);
		//void SearchListTreeChecked(TGListTreeItem* current, int &histneeded);

		void GetAllHistos(vector<hid_t> &hids);	
		//void GetHistsFromListTree(vector<hid_t> &hists); // first=server, second=hnamepath
		//void AddBranchToHists(TGListTreeItem *item, multimap<string,string> &hists);

	protected:
	
#if !defined(__CINT__) && !defined(__CLING__)
		rs_info::viewStyle_t viewStyle;
#endif //__CINT__  __CLING__
		
		void ParseFullPath(string &path, vector<string> &dirs);
		void UpdateListTree(vector<hid_t> hids);
	
	private:

		TTimer *timer;
		long sleep_time; // in milliseconds
		time_t last_called;
		time_t last_ping_time;
		time_t last_hist_time;
		vector<hid_t> last_hids;
		map<hid_t, TGListTreeItem*> server_items;
		map<hid_t, TGListTreeItem*> hist_items;

		void CreateGUI(void);

		TGCanvas *canvas;
		TGListTree *listTree;
		TGRadioButton *viewByObject;
		TGRadioButton *viewByServer;
		
		const TGPicture *folder_t;
		const TGPicture *ofolder_t;
		const TGPicture *h1_s;
		const TGPicture *h1_t;
		const TGPicture *h2_s;
		const TGPicture *h2_t;
		const TGPicture *h3_s;
		const TGPicture *h3_t;
		const TGPicture *pack_t;
		const TGPicture *hdisk_t;
		const TGPicture *checked_t;
		const TGPicture *unchecked_t;
		//JustinB Boolean that is read within DoTimer 
		//of Dialog_SaveHists to know when to save.
		bool tosave;
		bool displaybar;
		//Justin B. For DoSave().
		TGFileInfo* thefileinfo;
		//Justin B. For DoSave().
		int hist_get_size;
		ClassDef(Dialog_SaveHists,1)
		map<string, int> histtosave;
		TGVerticalFrame* verticalframe;
		TGHProgressBar *progressbar;
		TGMainFrame *mainframe;
		
};

#endif // _Dialog_SaveHists_

