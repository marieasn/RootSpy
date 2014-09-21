// $Id$
//
//    File: Dialog_ScaleOpts.cc
// Creator: sdobbs
//

#include <algorithm>
#include <iostream>
#include <sstream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_ScaleOpts.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"

#include <TApplication.h>

#include <TGLabel.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGButtonGroup.h>
#include <TGTextEntry.h>
#include <TTimer.h>
#include <TFile.h>
#include <TDirectory.h>
#include <unistd.h>
#include <TGFileDialog.h>

extern rs_mainframe *RSMF;

const string NEW_ENTRY_STR = "<new entry>";


enum {
    B_SCALE_NONE,
    B_SCALE_ALL,
    B_SCALE_BIN,
    B_SCALE_PERCENT
};

//---------------------------------
// Dialog_ScaleOpts    (Constructor)
//---------------------------------
Dialog_ScaleOpts::Dialog_ScaleOpts(string hnamepath, const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	this->hnamepath = hnamepath;
	this->Nbins = 1;
	
	double min = 0;
	double max = 100;

	// Get current min/max setting and Nbins in this histogram
	RS_INFO->Lock();
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
	if( hdef_iter != RS_INFO->histdefs.end() ) {
    	hdef_t *hdef = &hdef_iter->second;
		
		selected_mode = hdef->display_info.overlay_scale_mode;
		min = hdef->display_info.scale_range_low;
		max = hdef->display_info.scale_range_hi;
		if(hdef->sum_hist) Nbins = hdef->sum_hist->GetNbinsX();

	}
	RS_INFO->Unlock();

    // Define all of the of the graphics objects. 
	CreateGUI();

	// Initialize min/max
	stringstream ssmin;
	ssmin << min;
	stringstream ssmax;
	ssmax << max;
	minBox->SetText(ssmin.str().c_str());
	maxBox->SetText(ssmax.str().c_str());
	
	// Set selected mode (may also cap min and max)
	group->SetButton(selected_mode);
	
    // Finish up and map the window
	string title = "RootSpy Scaling Options: "+hnamepath;
    SetWindowName(title.c_str());
	MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

}

//---------------------------------
// ~Dialog_ScaleOpts    (Destructor)
//---------------------------------
Dialog_ScaleOpts::~Dialog_ScaleOpts()
{
	RSMF->dialog_scaleopts = NULL;
}

//---------------------------------
// CloseWindow
//---------------------------------
void Dialog_ScaleOpts::CloseWindow(void) {
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
}


//---------------------------------
// DoOk
//---------------------------------
void Dialog_ScaleOpts::DoOK(void)
{

	RS_INFO->Lock();

	// Get pointer to hdef_t
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
	if( hdef_iter == RS_INFO->histdefs.end() ) {
		cerr << "Cannot find histogram definition for \""<<hnamepath<<"\"!" << endl;
	}else{

		hdef_t *hdef = &hdef_iter->second;

		// save mode info
		hdef->display_info.overlay_scale_mode = selected_mode;

		// Set min and mac depending on mode selected
		double min = atoi(minBox->GetText());  // fragile!
		double max = atoi(maxBox->GetText());
		if(selected_mode == B_SCALE_PERCENT) {

			if(min < 0.)  min = 0.;
			if(max > 100.) max = 100.;

		} else if(selected_mode == B_SCALE_BIN) {

			if(min < 0.)  min = 0.;
			if(max > Nbins+1) max = Nbins+1;
		}

		hdef->display_info.scale_range_low = min;
		hdef->display_info.scale_range_hi = max;
	}
	
	RS_INFO->Unlock();

	CloseWindow();

}

//---------------------------------
// DoCancel
//---------------------------------
void Dialog_ScaleOpts::DoCancel(void)
{
    // quit without saving
    CloseWindow();
}


//---------------------------------
// DoRadioButton
//---------------------------------
void Dialog_ScaleOpts::DoRadioButton(Int_t id)
{	
	// Get current settings of min/max
	double min = atoi(minBox->GetText());
	double max = atoi(maxBox->GetText());

	// min can't be <0 in any case
	if(min<0) min = 0;

	// Adjust max based on mode
	switch(id) {
		case B_SCALE_PERCENT:
			if(max>100) max = 100;
			minBox->SetEnabled(kTRUE);
			maxBox->SetEnabled(kTRUE);
			break;
		case B_SCALE_BIN:
			if(max > Nbins+1) max = Nbins+1;
			minBox->SetEnabled(kTRUE);
			maxBox->SetEnabled(kTRUE);
			break;
		default:
			minBox->SetEnabled(kFALSE);
			maxBox->SetEnabled(kFALSE);
	}

	// Convert min/max to strings and copy into entries
	stringstream ssmin;
	ssmin << min;
	stringstream ssmax;
	ssmax << max;
	minBox->SetText(ssmin.str().c_str());
	maxBox->SetText(ssmax.str().c_str());

	// keep track of which button is currently selected
	selected_mode = id;
}

//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_ScaleOpts::CreateGUI(void)
{

    // main frame
    //TGMainFrame *fMainFrame844 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
    TGMainFrame *fMainFrame946 = this;
    //fMainFrame946->SetName("fMainFrame844");
    fMainFrame946->SetLayoutBroken(kTRUE);
   
    TGFont *ufont;         // will reflect user font changes
    ufont = gClient->GetFont("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1");

    TGGC   *uGC;           // will reflect user GC changes
    // graphics context changes
    GCValues_t vall572;
    vall572.fMask = kGCForeground | kGCBackground | kGCFillStyle | kGCFont | kGCGraphicsExposures;
    gClient->GetColorByName("#000000",vall572.fForeground);
    gClient->GetColorByName("#e0e0e0",vall572.fBackground);
    vall572.fFillStyle = kFillSolid;
    vall572.fFont = ufont->GetFontHandle();
    vall572.fGraphicsExposures = kFALSE;
    uGC = gClient->GetGC(&vall572, kTRUE);

    string label_str = "Nomalization Options";
    TGLabel *fLabel575 = new TGLabel(fMainFrame946,label_str.c_str(),uGC->GetGC());
    //fLabel575->SetTextJustify(36);
    fLabel575->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel575->SetMargins(0,0,0,0);
    fLabel575->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel575, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel575->MoveResize(10,15,0,0);
	
	stringstream ssbinrange;
	ssbinrange<<"Bin range(1-"<<Nbins<<")";

    group = new TGButtonGroup(fMainFrame946, "Scale Options");
    //horizontal->SetTitlePos(TGGroupFrame::kCenter);
    button1 = new TGRadioButton(group, "None", B_SCALE_NONE);
    button2 = new TGRadioButton(group, "Full range", B_SCALE_ALL);
    button3 = new TGRadioButton(group, ssbinrange.str().c_str(), B_SCALE_BIN);
    button4 = new TGRadioButton(group, "Percent range(1-100)", B_SCALE_PERCENT);

    //fMainFrame946->AddFrame(group, new TGLayoutHints(kLHintsExpandX));
    fMainFrame946->AddFrame(group, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    group->MoveResize(10,40,0,0);


    TGLabel *fLabel572 = new TGLabel(fMainFrame946,"Min",uGC->GetGC());
    fLabel572->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel572->SetMargins(0,0,0,0);
    fLabel572->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel572, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel572->MoveResize(200,60,0,0);
    TGLabel *fLabel573 = new TGLabel(fMainFrame946,"Max",uGC->GetGC());
    fLabel573->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel573->SetMargins(0,0,0,0);
    fLabel573->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel573, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel573->MoveResize(200,90,0,0);

    TGTextEntry *fTextEntry689 = new TGTextEntry(fMainFrame946, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
    fTextEntry689->SetMaxLength(3);
    fTextEntry689->SetAlignment(kTextLeft);
    fTextEntry689->SetText("");
    //fTextEntry689->Resize(280,fTextEntry689->GetDefaultHeight());
    fTextEntry689->MoveResize(240,60,45,fTextEntry689->GetDefaultHeight());
    fMainFrame946->AddFrame(fTextEntry689, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

    TGTextEntry *fTextEntry688 = new TGTextEntry(fMainFrame946, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
    fTextEntry688->SetMaxLength(3);
    fTextEntry688->SetAlignment(kTextLeft);
    fTextEntry688->SetText("");
    //fTextEntry688->Resize(280,fTextEntry688->GetDefaultHeight());
    fTextEntry688->MoveResize(240,90,45,fTextEntry688->GetDefaultHeight());
    fMainFrame946->AddFrame(fTextEntry688, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));


    TGTextButton *fTextButton558 = new TGTextButton(fMainFrame946,"OK");
    fTextButton558->SetTextJustify(36);
    fTextButton558->SetMargins(0,0,0,0);
    fTextButton558->SetWrapLength(-1);
    fTextButton558->Resize(92,24);
    fMainFrame946->AddFrame(fTextButton558, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fTextButton558->MoveResize(230,146,92,24);
    TGTextButton *fTextButton563 = new TGTextButton(fMainFrame946,"Cancel");
    fTextButton563->SetTextJustify(36);
    fTextButton563->SetMargins(0,0,0,0);
    fTextButton563->SetWrapLength(-1);
    fTextButton563->Resize(92,24);
    fMainFrame946->AddFrame(fTextButton563, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fTextButton563->MoveResize(120,146,92,24);

    fMainFrame946->SetMWMHints(kMWMDecorAll,
			       kMWMFuncAll,
			       kMWMInputModeless);
    fMainFrame946->MapSubwindows();

    fMainFrame946->Resize(fMainFrame946->GetDefaultSize());
    fMainFrame946->MapWindow();
    fMainFrame946->Resize(340,180);
    

    TGTextButton* &ok_button = fTextButton558;
    TGTextButton* &cancel_button = fTextButton563;
    
    this->minBox = fTextEntry689;
    this->maxBox = fTextEntry688;

    group->Connect("Pressed(Int_t)", "Dialog_ScaleOpts", this, "DoRadioButton(Int_t)");
    
    ok_button->Connect("Clicked()","Dialog_ScaleOpts", this, "DoOK()");
    cancel_button->Connect("Clicked()","Dialog_ScaleOpts", this, "DoCancel()");

    // initialize dialog values
    //group->SetButton(B_SCALE_ALL);
    //selected_mode = B_SCALE_ALL;
//    selected_mode = current_hdef_iter->second.display_info.overlay_scale_mode;
//    group->SetButton(selected_mode = B_SCALE_ALL);

//    if ( (selected_mode == B_SCALE_PERCENT) 
//	 || (selected_mode == B_SCALE_BIN) ) {
//	stringstream ss;
//	ss << current_hdef_iter->second.display_info.scale_range_low;
//	minBox->SetText(ss.str().c_str());
//	ss.str("");   ss.clear();
//	ss << current_hdef_iter->second.display_info.scale_range_hi;
//	maxBox->SetText(ss.str().c_str());
//    }
    
}

