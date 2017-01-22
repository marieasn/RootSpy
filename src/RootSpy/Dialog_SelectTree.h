

#ifndef _Dialog_SelectTree_
#define _Dialog_SelectTree_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListTree.h>
#include <TGPicture.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <TGFileDialog.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "../libRootSpy-client/hinfo_t.h"
#include "../libRootSpy-client/tree_info_t.h"

using namespace std;

class Dialog_SelectTree:public TGMainFrame{
	public:
		Dialog_SelectTree(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_SelectTree();


		void DoTimer(void);
		void CloseWindow(void);
		void DoDoubleClickedEntry(TGListTreeItem* entry, Int_t btn);

	protected:

#if !defined(__CINT__) && !defined(__CLING__)
		rs_info::viewStyle_t viewStyle;
#endif //__CINT__  __CLING__

		void ParseFullPath(string &path, vector<string> &dirs);
		void updateListTree(vector<vector<tree_info_t> > &tree_ids);
		void updateListTreeServer(map<string, server_info_t> &servinfo);

	private:
		void CreateGUI(void);
		void getAllTrees(vector<vector<tree_info_t> > &tree_ids);
		long sleep_time;
		TTimer *timer;
		TGCanvas *canvas;
		TGListTree *list_tree;
		TGRadioButton *viewByObject;
		TGRadioButton *viewByServer;
		TGTextButton *cancel;

		const TGPicture *folder_t;
		const TGPicture *ofolder_t;
		const TGPicture *tree_s;
		const TGPicture *tree_t;
		const TGPicture *pack_t;
		const TGPicture *hdisk_t;
		const TGPicture *checked_t;
		const TGPicture *unchecked_t;

		map<tree_id_t, TGListTreeItem*> server_items;
		map<tree_id_t, TGListTreeItem*> tree_items;

		time_t last_called;
		time_t last_ping_time;
		time_t last_hist_time;
		bool requested;
		unsigned int lastsize;
		ClassDef(Dialog_SelectTree,1)
};

#endif // _Dialog_SelectTree_

