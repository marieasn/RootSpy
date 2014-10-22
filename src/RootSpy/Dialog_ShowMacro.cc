// $Id$
// $HeadURL$
//
//    File: Dialog_ShowMacro.cc
// Creator: davidl

#include <stdint.h>

#include <algorithm>
#include <iostream>
#include <sstream>
using namespace std;

#include "Dialog_ShowMacro.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGButton.h>
#include <TGResourcePool.h>
#include <unistd.h>



//---------------------------------
// Dialog_ShowMacro    (Constructor)
//---------------------------------
Dialog_ShowMacro::Dialog_ShowMacro(string title, string macro_str):TGMainFrame(gClient->GetRoot(),700, 800, kMainFrame | kVerticalFrame)
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
	lb->SetHeight(800);
	
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

	// Add all lines of macro
	istringstream macro_stream(macro_str);
	uint32_t i = 0;
	while(!macro_stream.eof()) {
		string s;
		getline(macro_stream, s);
		i++;
		
		for(uint32_t j=0; j<s.size(); j++) if(s[j]=='\t'){
			s[j] = ' ';
			s.insert(j, "   ");
		}
		
		char lineno[8];
		sprintf(lineno, "%03d ", i);
		s = lineno + s;

		TGTextLBEntry *entry = new TGTextLBEntry(lb->GetContainer(), new TGString(s.c_str()), i+1, uGC->GetGC(), ufont->GetFontStruct());
		lb->AddEntry((TGLBEntry *)entry, new TGLayoutHints(kLHintsTop | kLHintsLeft));
	}
	
	TGTextButton *bOK = new TGTextButton(fMain, "Close");
	bOK->SetTextJustify(36);
	bOK->SetMargins(0,0,0,0);
	bOK->SetWrapLength(-1);
	bOK->Resize(100,22);
	fMain->AddFrame(bOK, new TGLayoutHints( kLHintsBottom | kLHintsExpandX, 2,2,2,2));
	
	bOK->Connect("Clicked()","Dialog_ShowMacro", this, "CloseWindow()");
	
    // Finish up and map the window
    SetWindowName(title.c_str());
	MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

}

//---------------------------------
// ~Dialog_ShowMacro    (Destructor)
//---------------------------------
Dialog_ShowMacro::~Dialog_ShowMacro()
{

}

//---------------------------------
// CloseWindow
//---------------------------------
void Dialog_ShowMacro::CloseWindow(void)
{
  	DeleteWindow();
	UnmapWindow();
}


