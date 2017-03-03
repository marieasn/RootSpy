// $Id: Dialog_ReferencePlot.h 21852 2017-01-22 01:52:11Z davidl $

//    File: Dialog_ReferencePlot.h
// Creator: sdobbs


#ifndef _Dialog_ReferencePlot_
#define _Dialog_ReferencePlot_

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
#include <TGFileDialog.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "../libRootSpy-client/hinfo_t.h"

using namespace std;

class Dialog_ReferencePlot:public TGMainFrame{
	public:
		Dialog_ReferencePlot(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_ReferencePlot();

		static string MakeReferenceFilename(string &hnamepath);
		static string GetReferenceArchiveDir(void);

		void DoClose(void);
		void DoRedraw(void);
		void DoTimer(void);
		void CloseWindow(void);
		
		TCanvas *canvas;
		string hnamepath;
		string image_filename;
		string image_mod_time;

		TImage *timage;

		TGLabel *lab_hnamepath;
		TGLabel *lab_image_filename;
		TGLabel *lab_image_modtime;

	private:

		TGLabel* AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints);
		TGTextButton* AddButton(TGCompositeFrame* frame, string text, ULong_t hints);
		void CreateGUI(void);

		ClassDef(Dialog_ReferencePlot,1)
};

#endif // _Dialog_ReferencePlot_

