// $Id$

//    File: Dialog_SelectHists.h
// Created: Sun Sep 13 17:53:00 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)


#ifndef _Dialog_SelectHists_
#define _Dialog_SelectHists_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListTree.h>
#include <TGPicture.h>
#include <TGTextEntry.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <list>
#include <TGFileDialog.h>

#ifndef __CINT__
#include "rs_info.h"
#endif //__CINT__

#include "hinfo_t.h"
#include "RSTab.h"

using namespace std;

class Dialog_SelectHists:public TGMainFrame{
	public:
		Dialog_SelectHists(list<string> *hnamepaths, const TGWindow *p, UInt_t w, UInt_t h);
		Dialog_SelectHists(RSTab *rstab, const TGWindow *p, UInt_t w, UInt_t h);
		void Init(list<string> *hnamepaths);
		virtual ~Dialog_SelectHists();
		
		void DoTimer(void);
		void DoOK(void);
		void DoCancel(void);
		void DoClickedEntry(TGListTreeItem* entry, Int_t btn);
		void DoSelectSingleHist(TGListTreeItem* entry, Int_t btn);
		void DoCheckedEntry(TObject* obj, Int_t check);
		void DoSetViewByObject(void);
		void DoSetViewByServer(void);
		void GetAllHistos(vector<hid_t> &hids);
		void CloseWindow(void);
		void DoFilterHists(void);
		void DoSelectAll(void);
		void DoSelectNone(void);


	protected:
	
#ifndef __CINT__
		RSTab *rstab;
		list<string> *hnamepaths; // Where to store results
		rs_info::viewStyle_t viewStyle;
#endif //__CINT__
		
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
		TGTextEntry *filterTextBox;
		
		const TGPicture *folder_t;
		const TGPicture *ofolder_t;
		const TGPicture *redx_t;
		const TGPicture *package_t;
		const TGPicture *h1_s;
		const TGPicture *h1_t;
		const TGPicture *h2_s;
		const TGPicture *h2_t;
		const TGPicture *h3_s;
		const TGPicture *h3_t;
		const TGPicture *profile_t;
		const TGPicture *pack_t;
		const TGPicture *hdisk_t;
		const TGPicture *checked_t;
		const TGPicture *unchecked_t;
//		//JustinB Boolean that is read within DoTimer 
//		//of Dialog_SelectHists to know when to save.
//		bool tosave; 
//		//Justin B. For DoSave().
//		TGFileInfo* fileinfo;

		string filter_str;

		ClassDef(Dialog_SelectHists,1)
};

#endif // _Dialog_SelectHists_

