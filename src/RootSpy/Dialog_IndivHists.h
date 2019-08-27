// $Id$
//
//    File: Dialog_IndivHists.h
// Created: 07.09.10
// Creator: justinb 
//

#ifndef _Dialog_IndivHists_
#define _Dialog_IndivHists_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListTree.h>
#include <TCanvas.h>
#include <TGPicture.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <TGFileDialog.h>

#if !defined(__CINT__) && !defined(__CLING__)
#include "rs_info.h"
#endif //__CINT__  __CLING__

#include "../libRootSpy-client/hinfo_t.h"

using namespace std;

class Dialog_IndivHists:public TGMainFrame{
	public:
		Dialog_IndivHists(string hnamepath, const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_IndivHists();

		//Methods
		//=====================================
		void CloseWindow(void);
		void DoTimer(void);
		void DoGridLines(void);
		void DoTickMarks(void);
		void DoUpdate(void);
		void DoCombined(void);
		void DoDivided(void);
		bool HandleConfigureNotify(Event_t* event);

		enum display_style {
			COMBINED,
			DIVIDED
		};
		
		string hnamepath;
		bool last_enable_divided;

	private:
		//Methods
		//=====================================
		void CreateGUI(void);
		bool HistogramsReturned(void);
		void CombinedStyle(void);
		void DividedStyle(void);
		void RequestCurrentIndividualHistograms(void);
		void SetCanvasStyle(TVirtualPad *thecanvas);
		void DivideCanvas(unsigned int &row, unsigned int &col);
		//Instance Variables
		//=====================================
		TTimer* timer;
		bool requested;
		bool servers_changed;
		unsigned int num_servers;
		TCanvas* canvas;
		TGCheckButton* gridlines;
		TGCheckButton* tickmarks;
		TGCheckButton* plainbut;
		TGRadioButton* combinedbut;
		TGRadioButton* dividedbut;
		TGVerticalFrame *verticalframe;
		display_style displaystyle;
		bool tickstyle;
		bool gridstyle;

		//=====================================
		ClassDef(Dialog_IndivHists,1)

	protected:
		void ParseFullPath(string &path, vector<string> &dirs);
};

#endif // _Dialog_IndivHists_

