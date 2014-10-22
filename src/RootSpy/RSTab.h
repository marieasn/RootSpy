// $Id$
//
//    File: RSTab.h
// Created: Sat Sep 13 19:34:26 EDT 2014
// Creator: davidl (on Darwin harriet.local 13.3.0 i386)
//

#ifndef _RSTab_
#define _RSTab_

#include <string>
#include <list>
#include <iostream>
using namespace std;

#include <TROOT.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TGTab.h>
#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TGMsgBox.h>
#include <TBrowser.h>
#include <TArrow.h>
#include <TLatex.h>
#include <TColor.h>
#include <TCanvas.h>

class rs_mainframe;

class RSTab{
	public:
		RSTab(rs_mainframe *rsmf, string title);
		RSTab(){}
		virtual ~RSTab();
		
		TGTab *fTab; // pointer to the TGTab object containing this tab
		TGCompositeFrame *fTabMain; // top-level frame for this tab
		
		TGVerticalFrame *fTabMainLeft;
		TGLabel *lServer;
		TGLabel *lType;
		TGLabel *lHistogram;
		TGLabel *lReceived;
		TGTextButton *bViewMacro;
		TCanvas *canvas;
		
		string title;
		string config;
		list<string> hnamepaths; // histos and macros to loop through showing
		int currently_displayed; // index of element in hnamepaths
		double currently_displayed_modified; // Time at which the currently displayed histo was last modified
		double last_update; // last time when DoUpdate() was called
		double last_request_sent;
		string last_request_hnamepath;
		bool hnamepaths_seeded; // set to true the first time we see hnamepaths is not empty
		
		void SeedHistos(void);
		void SetTo(string hnamepath);
		
		void DoNext(void);
		void DoPrev(void);
		void DoShowIndiv(void);
		void DoSaveCanvas(void);
		void DoUpdate(void);
		void DoUpdateWithFollowUp(void);
		void DoSelectHists(void);
		void DoOverlay(void);
		void DoReset(void);
		void DoRestore(void);
		void DoViewMacro(void);
		
		void GetMacroHists(vector<string> &hnamepaths);
		void RequestUpdatedMacroHists(void);
	
	protected:

		TGLabel*          AddLabel(TGCompositeFrame* frame, string text, Int_t mode=kTextLeft, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGTextButton*     AddButton(TGCompositeFrame* frame, string text, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGCheckButton*    AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints=kLHintsLeft | kLHintsTop);
		TGPictureButton*  AddPictureButton(TGCompositeFrame* frame, string picture, string tooltip="", ULong_t hints=kLHintsLeft | kLHintsTop);
		TGFrame*          AddSpacer(TGCompositeFrame* frame, UInt_t w=10, UInt_t h=10, ULong_t hints=kLHintsCenterX | kLHintsCenterY);
	
	private:


	ClassDef(RSTab,1)
};

#endif // _RSTab_

