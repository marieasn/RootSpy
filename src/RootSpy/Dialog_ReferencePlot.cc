// $Id: Dialog_ReferencePlot.cc 15950 2017-01-24 09:42:00Z davidl $
//
//    File: Dialog_ReferencePlot.cc
// Creator: sdobbs
//

#include <sys/stat.h>

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_ReferencePlot.h"
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

string REF_PLOTS_DIR = "/tmp";

//---------------------------------
// Dialog_ReferencePlot    (Constructor)
//---------------------------------
Dialog_ReferencePlot::Dialog_ReferencePlot(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
	cout << "Dialog_ReferencePlot() constructor..." << endl;
	
	canvas = NULL;
	timage = NULL;

	// Define all of the of the graphics objects. 
	CreateGUI();

	// Finish up and map the window
	SetWindowName("RootSpy Reference Plot");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

//---------------------------------
// ~Dialog_ReferencePlot    (Destructor)
//---------------------------------
Dialog_ReferencePlot::~Dialog_ReferencePlot()
{
	cout<<"Dialog_ReferencePlot destructor"<<endl;
}

//---------------------------------
// CloseWindow
//---------------------------------
void Dialog_ReferencePlot::CloseWindow(void)
{
	RSMF->dialog_referenceplot = NULL;
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
}

//-------------------
// AddLabel
//-------------------
TGLabel* Dialog_ReferencePlot::AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints)
{
	TGLabel *lab = new TGLabel(frame, text.c_str());
	lab->SetTextJustify(mode);
	lab->SetMargins(0,0,0,0);
	lab->SetWrapLength(-1);
	frame->AddFrame(lab, new TGLayoutHints(hints,2,2,2,2));

	return lab;
}

//-------------------
// AddButton
//-------------------
TGTextButton* Dialog_ReferencePlot::AddButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGTextButton *b = new TGTextButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	b->Resize(100,22);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}


//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_ReferencePlot::CreateGUI(void)
{

	// Main vertical frame
	TGVerticalFrame *fMain = new TGVerticalFrame(this);
	this->AddFrame(fMain, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,2,2,2));

	// Info labels
	TGHorizontalFrame *fMainTop = new TGHorizontalFrame(fMain);
	fMain->AddFrame(fMainTop, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
	TGVerticalFrame *fLabelsLeft = new TGVerticalFrame(fMainTop);
	fMainTop->AddFrame(fLabelsLeft, new TGLayoutHints(kLHintsLeft | kLHintsTop| kLHintsExpandX,2,2,2,2));
	TGVerticalFrame *fLabelsRight = new TGVerticalFrame(fMainTop);
	fMainTop->AddFrame(fLabelsRight, new TGLayoutHints(kLHintsLeft | kLHintsTop| kLHintsExpandX,2,2,2,2));
	
	AddLabel(fLabelsLeft, "Currently displayed:       ", kTextRight, kLHintsRight| kLHintsExpandX); 
	AddLabel(fLabelsLeft, "Image file:    ", kTextRight, kLHintsRight| kLHintsExpandX);
	AddLabel(fLabelsLeft, "Modified:    ", kTextRight, kLHintsRight| kLHintsExpandX);
	
	hnamepath      = "-----------------------------------------------------------------------------";
	image_filename = "-----------------------------------------------------------------------------";
	image_mod_time = "-----------------------------------------------------------------------------";
	lab_hnamepath      = AddLabel(fLabelsRight, hnamepath.c_str(), kTextLeft, kLHintsLeft); 
	lab_image_filename = AddLabel(fLabelsRight, image_filename.c_str(), kTextLeft, kLHintsLeft);
	lab_image_modtime  = AddLabel(fLabelsRight, image_mod_time.c_str(), kTextLeft, kLHintsLeft);

	//...... Embedded canvas ......
	TRootEmbeddedCanvas *embeddedcanvas = new TRootEmbeddedCanvas(0,fMain,600,400);	
	Int_t id = embeddedcanvas->GetCanvasWindowId();
	canvas = new TCanvas("referencePlotCanvas", 10, 10, id);
	canvas->SetTicks();
	canvas->SetDoubleBuffer();           // for smoother updates
	canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
	embeddedcanvas->AdoptCanvas(canvas);
	fMain->AddFrame(embeddedcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

	canvas->cd();
	timage = TImage::Create();
	timage->Scale(canvas->GetWw(), canvas->GetWh());
	timage->Draw();

	// Bottom frame
	TGHorizontalFrame *fMainBot = new TGHorizontalFrame(fMain);
	fMain->AddFrame(fMainBot, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX,2,2,2,2));
	TGTextButton *bClose = AddButton(fMainBot, "Close", kLHintsRight);
	bClose->Connect("Clicked()","Dialog_ReferencePlot", this, "DoClose()");
	
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

//---------------------------------
// MakeReferenceFilename
//---------------------------------
string Dialog_ReferencePlot::MakeReferenceFilename(string &hnamepath)
{
	/// This is used to convert an hnamepath into the full path
	/// to a .png file that would represent the reference plot
	/// for that hnamepath. It is used both by this class (see DoTimer)
	/// and rs_mainframe (see DoMakeReferencePlot)

	static bool initialized = false;
	if(!initialized){
		const char *ROOTSPY_REF_DIR = getenv("ROOTSPY_REF_DIR");
		if(ROOTSPY_REF_DIR) REF_PLOTS_DIR = ROOTSPY_REF_DIR;
		cout << "Reference histograms will be read from: " << REF_PLOTS_DIR << endl;
		initialized = true;
	}
	
	string fname = hnamepath;
	replace(fname.begin(), fname.end(), '/', '_');
	replace(fname.begin(), fname.end(), ':', '_');
	replace(fname.begin(), fname.end(), ' ', '-');
	fname = REF_PLOTS_DIR + "/" + fname + ".png";
	
	return fname;
}

//---------------------------------
// GetReferenceArchiveDir
//---------------------------------
string Dialog_ReferencePlot::GetReferenceArchiveDir(void)
{
	/// This will return a string containing the full path to the
	/// directory used to hold old reference plot image files. It
	/// is here so we can access the REF_PLOTS_DIR global just like
	/// above, even though it will be called from rs_mainframe
	/// DoMakeReferencePlot().
	string dirname = REF_PLOTS_DIR + "/ARCHIVE";
	
	return dirname;
}

//---------------------------------
// DoClose
//---------------------------------
void Dialog_ReferencePlot::DoClose(void)
{
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();
}

//-------------------
// DoTimer
//-------------------
void Dialog_ReferencePlot::DoTimer(void)
{
	// this is called from rs_mainframe::DoTimer if this dialog
	// has been instantiated AND is currently mapped.

	// Check if hnamepath has changed and if so, update everything
	string &hnp = RSMF->current_tab->currently_displayed_hnamepath;
	if(hnamepath!=hnp){
		hnamepath = hnp;
		string hnamepath_padded = hnamepath;
		hnamepath_padded += string(hnamepath.length()/2, ' ');
		lab_hnamepath->SetText(TString(hnamepath_padded));
		
		string fname = MakeReferenceFilename(hnamepath);
		if(fname != image_filename){
			string fname_padded = fname;
			fname_padded += string(fname.length()/2, ' ');
			lab_image_filename->SetText(TString(fname_padded));
			image_filename = fname;
			
			// check if image file exists
			bool file_exists = (access(fname.c_str(), F_OK) != -1);
			if(!file_exists){
				fname = REF_PLOTS_DIR + "/NO_REFERENCE.png";
				file_exists = (access(fname.c_str(), F_OK) != -1);
			}
			
			canvas->cd();
			if(file_exists){
			
				struct stat mystat;
				stat(image_filename.c_str(), &mystat);
				auto ftime = localtime(&mystat.st_mtime);
				char tmbuf[256];
				strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", ftime);
				image_mod_time = tmbuf;
				string image_mod_time_padded = image_mod_time;
				image_mod_time_padded += string(image_mod_time.length()/2, ' ');
				lab_image_modtime->SetText(TString(image_mod_time_padded));

				cout << "Loading reference image: " << fname << endl;
				timage->ReadImage(fname.c_str());
				//timage->Scale(canvas->GetWw(), canvas->GetWh());
				timage->Draw();
			}else{
				canvas->Clear();
			}

			canvas->Update();
			
// 			ifstream ifs(fname.cstr());
// 			if(ifs.is_open()){
// 				
// 			}
			
		}
	}
}


