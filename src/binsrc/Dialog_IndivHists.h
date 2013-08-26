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

#ifndef __CINT__
#include "rs_info.h"
#endif //__CINT__

#include "hinfo_t.h"

using namespace std;

class Dialog_IndivHists:public TGMainFrame{
	public:
		Dialog_IndivHists(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_IndivHists();

		//Methods
		//=====================================
		void CloseWindow(void);
		void DoTimer(void);
		void DoGridLines(void);
		void DoTickMarks(void);
		void DoPlainCanvas(void);
		void DoUpdate(void);
		void DoCombined(void);
		void DoDivided(void);
		Bool_t HandleConfigureNotify(Event_t* event);

		enum display_style {
			COMBINED,
			DIVIDED
		};

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
		bool display;
		bool servers_changed;
		unsigned int num_servers;
		TCanvas* canvas;
		TGCheckButton* gridlines;
		TGCheckButton* tickmarks;
		TGCheckButton* plainbut;
		TGRadioButton* combinedbut;
		TGRadioButton* dividedbut;
		TGVerticalFrame *verticalframe;
//		canvas_style canvasstyle;
		display_style displaystyle;
		bool tickstyle;
		bool gridstyle;

		//=====================================
		ClassDef(Dialog_IndivHists,1)

	protected:
		void ParseFullPath(string &path, vector<string> &dirs);
};

#endif // _Dialog_IndivHists_

