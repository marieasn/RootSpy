// $Id: Dialog_ShowGuidance.cc$
// $HeadURL: https://halldsvn.jlab.org/repos/trunk/online/packages/RootSpy/src/RootSpy/Dialog_ShowGuidance.cc $
//
//    File: Dialog_ShowGuidance.cc
// Creator: davidl

#include <stdint.h>

#include <algorithm>
#include <iostream>
#include <sstream>
using namespace std;

#include "Dialog_ShowGuidance.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGButton.h>
#include <TGResourcePool.h>
#include <unistd.h>



//---------------------------------
// Dialog_ShowGuidance    (Constructor)
//---------------------------------
Dialog_ShowGuidance::Dialog_ShowGuidance(string title, string guidance_str):TGMainFrame(gClient->GetRoot(),700, 700, kMainFrame | kVerticalFrame)
{
	TGVerticalFrame *fMain = new TGVerticalFrame(this);
	this->AddFrame(fMain, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY  ,2,2,2,2));	

	TGLabel *lab = new TGLabel(fMain, title.c_str());
	lab->SetTextJustify(kTextLeft);
	lab->SetMargins(0,0,0,0);
	lab->SetWrapLength(-1);
	fMain->AddFrame(lab, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX ,2,2,2,2));

	TGListBox *lb = new TGListBox(fMain);
	fMain->AddFrame(lb, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY  ,2,2,2,2));
	lb->SetWidth(600);
	lb->SetHeight(600);
	
	// We must go to some trouble to use a fixed-width font in the
	// listbox, but this is needed to get things to line up. This
	// was copied from an example here:
	// http://root.cern.ch/phpBB3/viewtopic.php?p=9281
	const TGFont *ufont;         // will reflect user font changes
	ufont = gClient->GetFont("-adobe-courier-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
	if (!ufont) ufont = fClient->GetResourcePool()->GetDefaultFont();

	// graphics context changes
	GCValues_t val;
	val.fMask = kGCFont;
	val.fFont = ufont->GetFontHandle();
	TGGC *uGC = gClient->GetGC(&val, kTRUE);

	// Provide default guidance if not explicitly given
	if(guidance_str==""){
		guidance_str = ""
		"No explicit guidance available for this plot\n"
		"If you have concerns regarding this then please\n"
		"contact the appropriate system expert, the run\n"
		"coordinator, or David Lawrence (x5567)";
	}
	
	// Preamble
	stringstream preamble;
	preamble << endl;
	preamble << "This guidance corresponds to: " << endl;
	preamble << endl;
	preamble << title << endl;
	preamble << endl;
	preamble << "To see the full macro contents hit the \"view\" button" << endl;
	preamble << "on the main RootSpy window." << endl;
	preamble << "NOTE: This DOES NOT update when the RootSpy window advances!" << endl;
	preamble << endl;
	preamble << "==============================================================" << endl;
	preamble << endl;
	

	// Add all lines of guidance
	istringstream guidance_stream(preamble.str() + guidance_str);
	uint32_t i = 0;
	while(!guidance_stream.eof()) {
		string s;
		getline(guidance_stream, s);
		i++;
		
		for(uint32_t j=0; j<s.size(); j++) if(s[j]=='\t'){
			s[j] = ' ';
			s.insert(j, "   ");
		}
		
		TGTextLBEntry *entry = new TGTextLBEntry(lb->GetContainer(), new TGString(s.c_str()), i+1, uGC->GetGC(), ufont->GetFontStruct());
		lb->AddEntry((TGLBEntry *)entry, new TGLayoutHints(kLHintsTop | kLHintsLeft));
	}
	
	TGTextButton *bOK = new TGTextButton(fMain, "Close");
	bOK->SetTextJustify(36);
	bOK->SetMargins(0,0,0,0);
	bOK->SetWrapLength(-1);
	bOK->Resize(100,22);
	fMain->AddFrame(bOK, new TGLayoutHints( kLHintsBottom | kLHintsExpandX, 2,2,2,2));
	
	bOK->Connect("Clicked()","Dialog_ShowGuidance", this, "CloseWindow()");
	
	// Finish up and map the window
	SetWindowName(title.c_str());
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//---------------------------------
// ~Dialog_ShowGuidance    (Destructor)
//---------------------------------
Dialog_ShowGuidance::~Dialog_ShowGuidance()
{

}

//---------------------------------
// CloseWindow
//---------------------------------
void Dialog_ShowGuidance::CloseWindow(void)
{
  	DeleteWindow();
	UnmapWindow();
}


