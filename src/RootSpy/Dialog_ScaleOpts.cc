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
Dialog_ScaleOpts::Dialog_ScaleOpts(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
    cout << " in Dialog_ScaleOpts constructor..." << endl;

    // make sure that there is a currently selected histogram, otherwise we bail
    // probably should throw up an error message if we do...
    RS_INFO->Lock();
    current_hdef_iter = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
    if( (RS_INFO->current.hnamepath == "")
        || (current_hdef_iter == RS_INFO->histdefs.end()) ) {
        RS_INFO->Unlock();
	RSMF->delete_dialog_scaleopts = true;
        return;
    }
        RS_INFO->Unlock();


    // Define all of the of the graphics objects. 
    CreateGUI();
    
    // Finish up and map the window
    SetWindowName("");
    //SetIconName("SelectHist");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

}

//---------------------------------
// ~Dialog_ScaleOpts    (Destructor)
//---------------------------------
Dialog_ScaleOpts::~Dialog_ScaleOpts()
{
	cout<<"escaped destructor"<<endl;
}


void Dialog_ScaleOpts::CloseWindow(void) {
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();

	RSMF->delete_dialog_scaleopts = true;
}


//---------------------------------
// DoOk
//---------------------------------
void Dialog_ScaleOpts::DoOK(void)
{	
    // save information and then quit
    current_hdef_iter->second.display_info.overlay_scale_mode = selected_mode;

    // error check
    if(selected_mode == B_SCALE_PERCENT) {
	double min = atoi(minBox->GetText());  // fragile!
	double max = atoi(maxBox->GetText());

	if(min < 0.)  min = 0.;
	if(max > 100.) max = 100.;

	current_hdef_iter->second.display_info.scale_range_low = min;
	current_hdef_iter->second.display_info.scale_range_hi = max;
    } else if(selected_mode == B_SCALE_BIN) {
	double min = atoi(minBox->GetText());  // fragile!
	double max = atoi(maxBox->GetText());

	if(min < 0.)  min = 0.;
	if(max > current_hdef_iter->second.sum_hist->GetNbinsX()+1) 
	    max = current_hdef_iter->second.sum_hist->GetNbinsX()+1;

	current_hdef_iter->second.display_info.scale_range_low = min;
	current_hdef_iter->second.display_info.scale_range_hi = max;
    }

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
    // handle any needed changes when someone clicks on one of the radio buttons
    switch(id) {
    case B_SCALE_PERCENT:
	minBox->SetText("0");
	maxBox->SetText("100");
	break;
    case B_SCALE_BIN:
	RS_INFO->Lock();
	// make sure that there is a valid histogram loaded
	map<string,hdef_t>::iterator hdef_itr = RS_INFO->histdefs.find(RS_INFO->current.hnamepath);
	if( (RS_INFO->current.hnamepath != "")
	    && (hdef_itr != RS_INFO->histdefs.end()) ) {
	    minBox->SetText("1");
	    stringstream ss;
	    ss << hdef_itr->second.sum_hist->GetNbinsX();
	    maxBox->SetText(ss.str().c_str());
	}
	RS_INFO->Unlock();	    
	break;
    }

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

    string label_str = "Options for " + current_hdef_iter->second.hnamepath;
    TGLabel *fLabel575 = new TGLabel(fMainFrame946,label_str.c_str(),uGC->GetGC());
    //fLabel575->SetTextJustify(36);
    fLabel575->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel575->SetMargins(0,0,0,0);
    fLabel575->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel575, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel575->MoveResize(10,15,0,0);
    

    group = new TGButtonGroup(fMainFrame946, "Scale Options");
    //horizontal->SetTitlePos(TGGroupFrame::kCenter);
    button1 = new TGRadioButton(group, "None", B_SCALE_NONE);
    button2 = new TGRadioButton(group, "Full range", B_SCALE_ALL);
    button3 = new TGRadioButton(group, "Bin range", B_SCALE_BIN);
    button4 = new TGRadioButton(group, "Percent range", B_SCALE_PERCENT);

    //fMainFrame946->AddFrame(group, new TGLayoutHints(kLHintsExpandX));
    fMainFrame946->AddFrame(group, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    group->MoveResize(10,40,0,0);



    TGLabel *fLabel572 = new TGLabel(fMainFrame946,"Min",uGC->GetGC());
    fLabel572->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel572->SetMargins(0,0,0,0);
    fLabel572->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel572, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel572->MoveResize(180,60,0,0);
    TGLabel *fLabel573 = new TGLabel(fMainFrame946,"Max",uGC->GetGC());
    fLabel573->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel573->SetMargins(0,0,0,0);
    fLabel573->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel573, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel573->MoveResize(180,90,0,0);

    TGTextEntry *fTextEntry689 = new TGTextEntry(fMainFrame946, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
    fTextEntry689->SetMaxLength(3);
    fTextEntry689->SetAlignment(kTextLeft);
    fTextEntry689->SetText("");
    //fTextEntry689->Resize(280,fTextEntry689->GetDefaultHeight());
    fTextEntry689->MoveResize(220,60,45,fTextEntry689->GetDefaultHeight());
    fMainFrame946->AddFrame(fTextEntry689, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

    TGTextEntry *fTextEntry688 = new TGTextEntry(fMainFrame946, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
    fTextEntry688->SetMaxLength(3);
    fTextEntry688->SetAlignment(kTextLeft);
    fTextEntry688->SetText("");
    //fTextEntry688->Resize(280,fTextEntry688->GetDefaultHeight());
    fTextEntry688->MoveResize(220,90,45,fTextEntry688->GetDefaultHeight());
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
    selected_mode = current_hdef_iter->second.display_info.overlay_scale_mode;
    group->SetButton(selected_mode);

    if ( (selected_mode == B_SCALE_PERCENT) 
	 || (selected_mode == B_SCALE_BIN) ) {
	stringstream ss;
	ss << current_hdef_iter->second.display_info.scale_range_low;
	minBox->SetText(ss.str().c_str());
	ss.str("");   ss.clear();
	ss << current_hdef_iter->second.display_info.scale_range_hi;
	maxBox->SetText(ss.str().c_str());
    }
    
}

