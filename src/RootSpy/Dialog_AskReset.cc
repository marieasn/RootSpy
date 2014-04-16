// $Id$
//
//    File: Dialog_AskReset.cc
// Creator: sdobbs
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_AskReset.h"
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

const string NEW_ENTRY_STR = "<new entry>";

//---------------------------------
// Dialog_AskReset    (Constructor)
//---------------------------------
Dialog_AskReset::Dialog_AskReset(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
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
// ~Dialog_AskReset    (Destructor)
//---------------------------------
Dialog_AskReset::~Dialog_AskReset()
{
	cout<<"escaped destructor"<<endl;
}


void Dialog_AskReset::CloseWindow(void) {
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();

	RSMF->delete_dialog_askreset = true;
}


//---------------------------------
// DoOk
//---------------------------------
void Dialog_AskReset::DoOK(void)
{	
    RS_INFO->Reset();
    CloseWindow();

}

//---------------------------------
// DoCancel
//---------------------------------
void Dialog_AskReset::DoCancel(void)
{
    CloseWindow();
}


//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_AskReset::CreateGUI(void)
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
    TGLabel *fLabel572 = new TGLabel(fMainFrame946,"Reset list of servers and histograms?",uGC->GetGC());
    //fLabel572->SetTextJustify(36);
    fLabel572->SetTextJustify( kTextLeft | kTextCenterY );
    fLabel572->SetMargins(0,0,0,0);
    fLabel572->SetWrapLength(-1);
    fMainFrame946->AddFrame(fLabel572, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fLabel572->MoveResize(50,20,344,32);
    TGTextButton *fTextButton558 = new TGTextButton(fMainFrame946,"OK");
    fTextButton558->SetTextJustify(36);
    fTextButton558->SetMargins(0,0,0,0);
    fTextButton558->SetWrapLength(-1);
    fTextButton558->Resize(92,24);
    fMainFrame946->AddFrame(fTextButton558, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fTextButton558->MoveResize(230,76,92,24);
    TGTextButton *fTextButton563 = new TGTextButton(fMainFrame946,"Cancel");
    fTextButton563->SetTextJustify(36);
    fTextButton563->SetMargins(0,0,0,0);
    fTextButton563->SetWrapLength(-1);
    fTextButton563->Resize(92,24);
    fMainFrame946->AddFrame(fTextButton563, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
    fTextButton563->MoveResize(120,76,92,24);

    fMainFrame946->SetMWMHints(kMWMDecorAll,
			       kMWMFuncAll,
			       kMWMInputModeless);
    fMainFrame946->MapSubwindows();

    fMainFrame946->Resize(fMainFrame946->GetDefaultSize());
    fMainFrame946->MapWindow();
    fMainFrame946->Resize(340,110);
    

    TGTextButton* &ok_button = fTextButton558;
    TGTextButton* &cancel_button = fTextButton563;
    
    ok_button->Connect("Clicked()","Dialog_AskReset", this, "DoOK()");
    cancel_button->Connect("Clicked()","Dialog_AskReset", this, "DoCancel()");
    
}

