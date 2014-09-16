// $Id$
//
//    File: Dialog_IndivHists.cc
// Created: 07.09.10
// Creator: justinb
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_IndivHists.h"
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
#include <TLegend.h>

//---------------------------------
// Dialog_IndivHists    (Constructor)
//---------------------------------
Dialog_IndivHists::Dialog_IndivHists(string hnamepath, const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	this->hnamepath = hnamepath;

	CreateGUI();
	timer = new TTimer();
	timer->Connect("Timeout()", "Dialog_IndivHists", this, "DoTimer()");
	timer->Start(500, kFALSE);
	requested = false;
	display = true;
	servers_changed = true;
	num_servers = RS_INFO->servers.size();
	displaystyle = COMBINED;
	tickstyle = true;
	gridstyle = true;

	//Set Divided Radio Button enabled or disabled.
	last_enable_divided = num_servers>1;
	dividedbut->SetEnabled(last_enable_divided);

	SetWindowName("Individual Histograms");
	SetIconName("Individual Histograms");
	MapSubwindows();
	MapWindow();
	Resize(GetDefaultSize());
}


//---------------------------------
// ~Dialog_IndivHists    (Destructor)
//---------------------------------
Dialog_IndivHists::~Dialog_IndivHists(){}

//=====================
// CloseWindow
//=====================
void Dialog_IndivHists::CloseWindow() {
	timer->Stop();
	delete timer;
	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
	RSMF->delete_dialog_indivhists = true;
}

//=====================
// DoTimer
//=====================
void Dialog_IndivHists::DoTimer(void) {

	// Enable/disable the "Divided" button depending on if we have more than 1 histogram
	Bool_t enable_divided = num_servers>1;
	if(enable_divided!= last_enable_divided) dividedbut->SetEnabled(enable_divided);
	last_enable_divided = enable_divided;

	// Request the histograms with RS_INFO->current's name from the
	// individual servers.
	if(!requested) {
		RequestCurrentIndividualHistograms();
	}

	// Draw histograms
	if(displaystyle == COMBINED) {
		CombinedStyle();
	} else { // displaystyle == DIVIDED
		DividedStyle();
	}

	canvas->Resize();  // force redraw
}

//============================
// SetCanvasStyle
//============================
void Dialog_IndivHists::SetCanvasStyle(TVirtualPad *thecanvas) {
	if(!gridstyle && !tickstyle) {
		//cout<<"plain"<<endl;
		thecanvas->SetTickx(0);
		thecanvas->SetTicky(0);
		thecanvas->SetGridx(0);
		thecanvas->SetGridy(0);
	}

	// Display a canvas with grid lines.
	if(gridstyle) {
		//cout<<"grid"<<endl;
		thecanvas->SetGridx(1);
		thecanvas->SetGridy(1);
	} else {
		thecanvas->SetGridx(0);
		thecanvas->SetGridy(0);
	}

	// Display a canvas with tick lines.
	if(tickstyle) {
		//cout<<"tick"<<endl;
		thecanvas->SetTickx(1);
		thecanvas->SetTicky(1);
	} else {
		thecanvas->SetTickx(0);
		thecanvas->SetTicky(0);
	}
}

//============================
// RequestCurrentIndividualHistograms
//============================
void Dialog_IndivHists::RequestCurrentIndividualHistograms(void) {
	RS_INFO->Lock();
	map<string, server_info_t>::iterator server_iter = RS_INFO->servers.begin();
	for(; server_iter != RS_INFO->servers.end(); server_iter++) {
		string server_name = (server_iter->second.serverName);
		RS_CMSG->RequestHistogram(server_name, RS_INFO->current.hnamepath);
	}
	RS_INFO->Unlock();
	requested = true;
	display = true;
}

//============================
// CombinedStyle
//============================
void Dialog_IndivHists::CombinedStyle(void) {
	// Display a plain canvas.
	canvas->cd();
	SetCanvasStyle(canvas);

	// Get pointer to hdef_t object
	RS_INFO->Lock();
	hdef_t histodef = RS_INFO->histdefs[hnamepath];

	// Make the Legend.
	double height = 0.1 + 0.05*(double)histodef.hists.size();
	if(height>0.75) height = 0.75;
	TLegend* legend = new TLegend(0.7, 0.895-height, 0.895, 0.895);
	//legend->SetHeader("Histograms");

	//Iterate by individual histogram over all the servers.
	map<string, hinfo_t>::iterator hists_iter = histodef.hists.begin();
	unsigned int color = 1;
	for(; hists_iter != histodef.hists.end(); hists_iter++) {
		TH1* histogram = hists_iter->second.hist;
		histogram->SetLineColor(color);
		if(color == 1) histogram->Draw();
		else histogram->Draw("same");

		//Dynamically add Histograms to the Legend.
		legend->AddEntry(histogram, hists_iter->second.serverName.c_str());
		//legend->AddEntry(histogram, NULL, "l");
		color++;
	}
	legend->Draw();
	canvas->Modified();
	canvas->Update();

	// When the timer is called, do not attempt to display.
	display = false;
	RS_INFO->Unlock();
}

//============================
// DivideCanvas
//============================
void Dialog_IndivHists::DivideCanvas(unsigned int &row, unsigned int &col) {

	// Calculate rows and columns to give something roughly even
	row = col = 1;
	while(row*col < num_servers){
		if(row >= col)
			row++;
		else
			col++;
	}
	
	canvas->Divide(col, row);
}


//============================
// DividedStyle
//============================
void Dialog_IndivHists::DividedStyle(void) {
    unsigned int row = 1;
    unsigned int col = 0;
    num_servers = RS_INFO->servers.size();
	canvas->Clear();
    DivideCanvas(row, col);
	// Divide the canvas appropriately.
	//canvas->Divide(col, row);
	//Iterate by individual histogram over all the servers.
	RS_INFO->Lock();
	hdef_t histodef = RS_INFO->histdefs[hnamepath];
	map<string, hinfo_t>::iterator hists_iter = histodef.hists.begin();
	unsigned int colors[] = {kRed, kBlue, kOrange, kBlack, kGreen, kViolet};
	unsigned int Ncolors = 6;
	unsigned int color = 0;
	unsigned int canvas_id = 1;  // divided canvas needs to start at 1 !
	TLatex latex;
	latex.SetTextSize(0.035);
	for(; hists_iter != histodef.hists.end(); hists_iter++) {
		TH1* histogram = hists_iter->second.hist;
		histogram->SetLineColor(colors[color%Ncolors]);
		TVirtualPad* subcanvas = canvas->cd(canvas_id++);
		SetCanvasStyle(subcanvas);
		histogram->Draw();

		double x = 0.0;
		double y = histogram->GetMaximum()*1.06;
		latex.DrawLatex(x, y, hists_iter->second.serverName.c_str());

		color++;
	}
	canvas->Modified();
	canvas->Update();

	// When the timer is called, do not attempt to display.
	display = false;
	RS_INFO->Unlock();
}

//============================
// HistogramsReturned
//============================
bool Dialog_IndivHists::HistogramsReturned(void) {
	RS_INFO->Lock();
	hdef_t histodef = RS_INFO->histdefs[RS_INFO->current.hnamepath];
	// Check to make sure hists, the map that holds all the individual
	// histograms contained in the sum, has a size equal to the number
	// of servers.
	if(histodef.hists.size() == RS_INFO->servers.size()) {
		RS_INFO->Unlock();
		return true;
	}
	RS_INFO->Unlock();
	return false;
}

//============================
// DoGridLines
//============================
void Dialog_IndivHists::DoGridLines(void) {
	if(gridlines->GetState() == kButtonUp) {
		gridstyle = false;
	} else {
		gridstyle = true;
	}
	display = true;
}

//============================
// DoTickMarks
//============================
void Dialog_IndivHists::DoTickMarks(void) {
	if(tickmarks->GetState() == kButtonUp) {
//		tickmarks->SetState(kButtonUp);
		tickstyle = false;
	} else {
		tickstyle = true;
	}
	display = true;
}

//============================
// DoUpdate
//============================
void Dialog_IndivHists::DoUpdate(void) {
	requested = false;
}

//============================
// DoCombined
//============================
void Dialog_IndivHists::DoCombined(void) {
	combinedbut->SetState(kButtonDown);
	dividedbut->SetState(kButtonUp);

	displaystyle = COMBINED;
	display = true;
}

//============================
// DoDivided
//============================
void Dialog_IndivHists::DoDivided(void) {
	combinedbut->SetState(kButtonUp);
	dividedbut->SetState(kButtonDown);

	displaystyle = DIVIDED;
	display = true;
}

Bool_t Dialog_IndivHists::HandleConfigureNotify(Event_t* event) {
	TGFrame::HandleConfigureNotify(event);
	return kTRUE;
}


//============================
// CreateGUI//
//============================
void Dialog_IndivHists::CreateGUI(void) {

	// Code generated by TGuiBuilder.
	// main frame
	TGMainFrame *fMainFrame2360 = this;

	   // composite frame
	   TGCompositeFrame *fMainFrame789 = new TGCompositeFrame(fMainFrame2360,879,678, kHorizontalFrame);

		   // vertical frame
		   TGVerticalFrame *fVerticalFrame790 = new TGVerticalFrame(fMainFrame789,133,676,kVerticalFrame | kRaisedFrame | kFitHeight);

			   TGTextButton *update_button = new TGTextButton(fVerticalFrame790,"Update");
			   update_button->SetTextJustify(36);
			   update_button->SetMargins(0,0,0,0);
			   update_button->SetWrapLength(-1);
			   update_button->Resize(99,24);
			   fVerticalFrame790->AddFrame(update_button, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

			   TGRadioButton *combinedbutton = new TGRadioButton(fVerticalFrame790,"Combined");
			   combinedbutton->SetState(kButtonDown);
			   combinedbutton->SetTextJustify(36);
			   combinedbutton->SetMargins(0,0,0,0);
			   combinedbutton->SetWrapLength(-1);
			   fVerticalFrame790->AddFrame(combinedbutton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

			   TGRadioButton *dividedbutton = new TGRadioButton(fVerticalFrame790,"Divided Canvas");
			   dividedbutton->SetState(kButtonUp);
			   dividedbutton->SetTextJustify(36);
			   dividedbutton->SetMargins(0,0,0,0);
			   dividedbutton->SetWrapLength(-1);
			   fVerticalFrame790->AddFrame(dividedbutton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

			  // Check Buttons
			  TGCheckButton *tickmarkbutton = new TGCheckButton(fVerticalFrame790,"Tick Marks");
			  tickmarkbutton->SetTextJustify(36);
			  tickmarkbutton->SetMargins(0,0,0,0);
			  tickmarkbutton->SetWrapLength(-1);
			  tickmarkbutton->SetState(kButtonDown);
			  fVerticalFrame790->AddFrame(tickmarkbutton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

			  TGCheckButton *gridlinebutton = new TGCheckButton(fVerticalFrame790,"Grid Lines");
			  gridlinebutton->SetTextJustify(36);
			  gridlinebutton->SetMargins(0,0,0,0);
			  gridlinebutton->SetWrapLength(-1);
			  gridlinebutton->SetState(kButtonDown);
			  fVerticalFrame790->AddFrame(gridlinebutton, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

			  TGTextButton *closebutton = new TGTextButton(fVerticalFrame790,"Close");
			  closebutton->SetTextJustify(36);
			  closebutton->SetMargins(0,0,0,0);
			  closebutton->SetWrapLength(-1);
			  fVerticalFrame790->AddFrame(closebutton, new TGLayoutHints(kLHintsBottom | kLHintsExpandX,2,2,2,2));

	   // Add Vertical Frame.
	   fMainFrame789->AddFrame(fVerticalFrame790, new TGLayoutHints(kLHintsRight | kLHintsTop | kLHintsExpandY,2,2,2,2));

		   // embedded canvas
		   TRootEmbeddedCanvas *fCanvas = new TRootEmbeddedCanvas(0,fMainFrame789,732,672);
		   Int_t wfCanvas = fCanvas->GetCanvasWindowId();
		   TCanvas *c125 = new TCanvas("c125", 10, 10, wfCanvas);
		   fCanvas->AdoptCanvas(c125);
		   fMainFrame789->AddFrame(fCanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY,2,2,2,2));

	// Add Composite Frame.
	fMainFrame2360->AddFrame(fMainFrame789, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

	fMainFrame2360->SetMWMHints(kMWMDecorAll,
						kMWMFuncAll,
						kMWMInputModeless);
	fMainFrame2360->MapSubwindows();

	fMainFrame2360->Resize(fMainFrame2360->GetDefaultSize());
	fMainFrame2360->MapWindow();
	fMainFrame2360->Resize(877,676);
	//====================================================================================================================

	canvas = fCanvas->GetCanvas();
	gridlines = gridlinebutton;
	tickmarks = tickmarkbutton;
	combinedbut = combinedbutton;
	dividedbut = dividedbutton;
	verticalframe = fVerticalFrame790;
	gridlines->Connect("Clicked()", "Dialog_IndivHists", this, "DoGridLines()");
	tickmarks->Connect("Clicked()", "Dialog_IndivHists", this, "DoTickMarks()");
	combinedbut->Connect("Clicked()", "Dialog_IndivHists", this, "DoCombined()");
	dividedbut->Connect("Clicked()", "Dialog_IndivHists", this, "DoDivided()");
	update_button->Connect("Clicked()", "Dialog_IndivHists", this, "DoUpdate()");
	closebutton->Connect("Clicked()", "Dialog_IndivHists", this, "CloseWindow()");
}
