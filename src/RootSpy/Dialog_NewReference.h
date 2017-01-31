// $Id: Dialog_NewReference.h 21852 2017-01-22 01:52:11Z davidl $

//    File: Dialog_NewReference.h
// Creator: sdobbs


#ifndef _Dialog_NewReference_
#define _Dialog_NewReference_

#include <TCanvas.h>
#include <TGLabel.h>
#include <TImage.h>
#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListBox.h>
#include <TGPicture.h>
#include <TGTextEntry.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <set>
#include <TGFileDialog.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "../libRootSpy-client/hinfo_t.h"

using namespace std;

class Dialog_NewReference:public TGMainFrame{
	public:
		Dialog_NewReference(const TGWindow *p, UInt_t w, UInt_t h, string hnamepath, string tmp_fname);
		virtual ~Dialog_NewReference();

		static string MakeReferenceFilename(string &hnamepath);
		static string GetReferenceArchiveDir(void);

		void DoCancel(void);
		void DoOK(void);
		void CloseWindow(void);
		
		string hnamepath;
		string image_filename_new;
		string image_filename_old;
		set<string> emails;
		string fname;
		string tmp_fname;

		TGCheckButton *cbagree;

	private:

		TGLabel* AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints);
		TGTextButton* AddButton(TGCompositeFrame* frame, string text, ULong_t hints);
		TGCheckButton* AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints);


		ClassDef(Dialog_NewReference,1)
};

#endif // _Dialog_NewReference_

