// $Id$
//
//    File: RSTab.cc
// Created: Sat Sep 13 19:34:26 EDT 2014
// Creator: davidl (on Darwin harriet.local 13.3.0 i386)
//

#include "RSTab.h"
#include "rs_mainframe.h"
#include "rs_info.h"
#include "rs_cmsg.h"
#include "Dialog_IndivHists.h"
#include "Dialog_SelectHists.h"

//---------------------------------
// RSTab    (Constructor)
//---------------------------------
RSTab::RSTab(rs_mainframe *rsmf, string title)
{
	//- - - - - - - - - - - - - - - - - - - - - - - - - 
	// Build all GUI elements in a single tab
	
	// Copy point to TGTab object to member variable
	fTab = rsmf->fMainTab;

	// Create new tab
	TGCompositeFrame *f = fTab->AddTab(title.c_str());
	this->title = title;
	
	// Hoizontal frame (controls to left, canvas to the right)
	fTabMain = new TGHorizontalFrame(f);
	f->AddFrame(fTabMain, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY ,2,2,2,2));
	
	//...... Controls ......
	fTabMainLeft = new TGVerticalFrame(fTabMain);
	fTabMain->AddFrame(fTabMainLeft, new TGLayoutHints(kLHintsLeft | kLHintsTop ,2,2,2,2));
	
	// Info. on what's currently displayed
	TGVerticalFrame *fTabMainLeftInfo = new TGVerticalFrame(fTabMainLeft);	
	fTabMainLeft->AddFrame(fTabMainLeftInfo, new TGLayoutHints(kLHintsCenterX | kLHintsTop | kLHintsExpandX,2,2,2,2));
	AddLabel(fTabMainLeftInfo, "Server:"    ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);
	lServer    = AddLabel(fTabMainLeftInfo, string(25, '-'),kTextRight | kLHintsExpandX);
	AddSpacer(fTabMainLeftInfo, 1, 5);
	AddLabel(fTabMainLeftInfo, "Histogram:" ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);
	lHistogram = AddLabel(fTabMainLeftInfo, string(25, '-'),kTextRight | kLHintsExpandX);
	AddSpacer(fTabMainLeftInfo, 1, 5);
	AddLabel(fTabMainLeftInfo, "Received:"  ,kTextLeft, kLHintsLeft | kLHintsTop | kLHintsExpandX);
	lReceived  = AddLabel(fTabMainLeftInfo, string(25, '-'),kTextRight | kLHintsExpandX);
	
	// Add some space between labels and controls
	AddSpacer(fTabMainLeft, 1, 10);
	
	// Previous, Next, and Refresh buttons
	TGHorizontalFrame *fTabMainLeftPrevNext = new TGHorizontalFrame(fTabMainLeft);
	TGPictureButton *bPrev = AddPictureButton(fTabMainLeftPrevNext, "GoBack.gif"   , "go to previous histogram");
	AddSpacer(fTabMainLeftPrevNext, 20, 1); // x-space between previous and next buttons
	TGPictureButton *bNext = AddPictureButton(fTabMainLeftPrevNext, "GoForward.gif", "go to next histogram");
	fTabMainLeft->AddFrame(fTabMainLeftPrevNext, new TGLayoutHints(kLHintsCenterX,2,2,2,2));
	TGPictureButton *bUpdate = AddPictureButton(fTabMainLeft, "ReloadPage.gif"   , "refresh current histogram", kLHintsCenterX);

	AddSpacer(fTabMainLeft, 1, 10); // y-space between prev/next and refresh

	// Display Type
	TGGroupFrame *fDisplayOptions = new TGGroupFrame(fTabMainLeft, "Display Options", kVerticalFrame);
	fTabMainLeft->AddFrame(fDisplayOptions, new TGLayoutHints(kLHintsExpandX | kLHintsCenterY, 2,2,2,2));
	TGTextButton *bSelect = AddButton(fDisplayOptions, "Select", kLHintsExpandX);
	
	// Buttons at bottom left
	TGVerticalFrame *fTabMainLeftBottom = new TGVerticalFrame(fTabMainLeft);
	TGTextButton *bShowIndiv = AddButton(fTabMainLeftBottom, "Show Individual", kLHintsTop| kLHintsExpandX);
	TGTextButton *bSaveCanvas = AddButton(fTabMainLeftBottom, "Save Canvas", kLHintsTop| kLHintsExpandX);	
	fTabMainLeft->AddFrame(fTabMainLeftBottom, new TGLayoutHints(kLHintsCenterX | kLHintsBottom| kLHintsExpandX, 2,2,2,2));
	
	//...... Embedded canvas ......
	char cname[256];
	static int cntr = 1;
	sprintf(cname, "canvas%02d", cntr++);
	TRootEmbeddedCanvas *embeddedcanvas = new TRootEmbeddedCanvas(0,fTabMain,900,600);
	Int_t id = embeddedcanvas->GetCanvasWindowId();
	canvas = new TCanvas(cname, 10, 10, id);
	canvas->SetTicks();
	embeddedcanvas->AdoptCanvas(canvas);
	fTabMain->AddFrame(embeddedcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY | kFitWidth | kFitHeight,2,2,2,2));

	rsmf->rstabs.push_back(this);
	rsmf->current_tab = this;
	fTab->SetTab(rsmf->rstabs.size() - 1);
	
	// ----------- Connect GUI elements to methods -----------
	bNext->Connect("Clicked()","RSTab", this, "DoNext()");
	bPrev->Connect("Clicked()","RSTab", this, "DoPrev()");
	bShowIndiv->Connect("Clicked()","RSTab", this, "DoShowIndiv()");
	bSaveCanvas->Connect("Clicked()","RSTab", this, "DoSaveCanvas()");
	bUpdate->Connect("Clicked()","RSTab", this, "DoUpdateWithFollowUp()");
	bSelect->Connect("Clicked()","RSTab", this, "DoSelectHists()");
	
	// Set some defaults
	config = title;
	currently_displayed = 0;
	currently_displayed_modified = 0.0;
	last_update = 0.0;
	last_request_sent = -10.0;
	last_servers_str_Nlines = 1;
	hnamepaths_seeded = false;
}

//---------------------------------
// ~RSTab    (Destructor)
//---------------------------------
RSTab::~RSTab()
{

}

//---------------------------------
// SeedHistos
//---------------------------------
void RSTab::SeedHistos(void)
{
	/// Get list of all currently defined histos and copy them
	/// into our hnamepath list, overwritting any that are
	/// currently there. This is used to seed the list when
	/// the tab is initially created. If the hnamepaths_seeded
	/// flag is set then this returns immediately without doing
	/// anything.

	if(hnamepaths_seeded) return;

	RS_INFO->Lock();

	// Make list of all histograms from all servers
	list<string> new_hnamepaths;
	map<string,hdef_t>::iterator it = RS_INFO->histdefs.begin();
	for(; it!=RS_INFO->histdefs.end(); it++){//iterates over servers

		new_hnamepaths.push_back(it->first);

	}
	
	RS_INFO->Unlock();
	
	hnamepaths = new_hnamepaths;

	// Tell the screen to update quickly
	if(!hnamepaths.empty()){
		hnamepaths_seeded = true;
		DoUpdateWithFollowUp();
	}
}

//-------------------
// SetTo
//-------------------
void RSTab::SetTo(string hnamepath)
{
	int idx = 0;
	list<string>::iterator it = hnamepaths.begin();
	for(; it!=hnamepaths.end(); it++, idx++){
		if(*it == hnamepath){
			if(idx != currently_displayed){
				currently_displayed = idx;
				TTimer::SingleShot(1, "RSTab", this, "DoUpdate()"); // have canvas updated quickly
				TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
				return;
			}
		}
	}
}

//-------------------
// AddLabel
//-------------------
TGLabel* RSTab::AddLabel(TGCompositeFrame* frame, string text, Int_t mode, ULong_t hints)
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
TGTextButton* RSTab::AddButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGTextButton *b = new TGTextButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	b->Resize(100,22);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddCheckButton
//-------------------
TGCheckButton* RSTab::AddCheckButton(TGCompositeFrame* frame, string text, ULong_t hints)
{
	TGCheckButton *b = new TGCheckButton(frame, text.c_str());
	b->SetTextJustify(36);
	b->SetMargins(0,0,0,0);
	b->SetWrapLength(-1);
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddPictureButton
//-------------------
TGPictureButton* RSTab::AddPictureButton(TGCompositeFrame* frame, string picture, string tooltip, ULong_t hints)
{
	TGPictureButton *b = new TGPictureButton(frame, gClient->GetPicture(picture.c_str()));
	if(tooltip.length()>0) b->SetToolTipText(tooltip.c_str());
	frame->AddFrame(b, new TGLayoutHints(hints,2,2,2,2));

	return b;
}

//-------------------
// AddSpacer
//-------------------
TGFrame* RSTab::AddSpacer(TGCompositeFrame* frame, UInt_t w, UInt_t h, ULong_t hints)
{
	/// Add some empty space. Usually, you'll only want to set w or h to
	/// reserve width or height pixels and set the other to "1".

	TGFrame *f =  new TGFrame(frame, w, h);
	frame->AddFrame(f, new TGLayoutHints(hints ,2,2,2,2));
	
	return f;
}

//-------------------
// DoNext
//-------------------
void RSTab::DoNext(void)
{
	currently_displayed++;
	if(currently_displayed >= hnamepaths.size()) currently_displayed = 0;
	DoUpdateWithFollowUp();
}

//-------------------
// DoPrev
//-------------------
void RSTab::DoPrev(void)
{
	currently_displayed--;
	if(currently_displayed < 0){
		if(hnamepaths.empty()){
			currently_displayed = 0;
		}else{
			currently_displayed = hnamepaths.size() -1;
		}
	}
	DoUpdateWithFollowUp();
}

//-------------------
// DoShowIndiv
//-------------------
void RSTab::DoShowIndiv(void)
{
	string hnamepath = lHistogram->GetText()->GetString();
	new Dialog_IndivHists(hnamepath, gClient->GetRoot(), 10, 10);
}

//----------
// DoSaveCanvas
//----------
//added by JustinBarry 06.14.10
//Save the current canvas as an image file.
void RSTab::DoSaveCanvas(void)
{
	TGFileInfo* fileinfo = new TGFileInfo();
	new TGFileDialog(gClient->GetRoot(), gClient->GetRoot(), kFDSave, fileinfo);
	canvas->SaveAs(fileinfo->fFilename, "");

	delete fileinfo;
}

//----------
// DoUpdate
//----------
void RSTab::DoUpdate(void)
{
	/// This is called by the rs_mainframe::DoTimer about every 250 milliseconds
	/// to refresh the canvas as needed. It is also called if the 

	double now = RS_CMSG->GetTime();

	// If we have no histograms in our list to display, clear the canvas
	// and return immediately. Only clear the canvas once every 3 seconds
	if(hnamepaths.empty()){
		if( (now-currently_displayed_modified) > 3.0 ){
			canvas->cd();
			canvas->Clear();
			canvas->Update();
			currently_displayed_modified = now;
		}
		
		// Seed hnamepaths when we first start up
		if(!hnamepaths_seeded) SeedHistos();

		return;
	}
	hnamepaths_seeded = true;

	// If the currently displayed histogram index is out of range, force it into range
	if(currently_displayed >= hnamepaths.size()) currently_displayed = 0;

	// Get hnamepath of currently displayed histo/macro
	list<string>::iterator h_it = hnamepaths.begin();
	advance(h_it, currently_displayed);
	string &hnamepath = *h_it;

	// Send request if at least 0.5 seconds has passed sent we last sent a request
	// or the thing we are displaying has changed
	if( ((now-last_request_sent) > 0.5) || (hnamepath != last_request_hnamepath) ){
		int Nrequests = RS_INFO->RequestHistograms(hnamepath);
		last_request_sent = now;
		last_request_hnamepath = hnamepath;
		//cout << "Sent " << Nrequests << " requests sent for " << hnamepath << endl;
		Nrequests++; // avoid compiler warnings if above line is commented out
	}
	
	// Lock ROOT
	pthread_rwlock_wrlock(ROOT_MUTEX);
	
	// Get Pointer to histogram to display the type of histogram (possibly a macro)
	hdef_t::histdimension_t type = hdef_t::noneD;
	double sum_hist_modified = 0.0;
	string servers_str;
	TH1* sum_hist = RS_INFO->GetSumHist(hnamepath, &type, &sum_hist_modified, &servers_str);
	
	// Update labels
	time_t t = (unsigned long)floor(sum_hist_modified+rs_cmsg::start_time); // seconds since 1970
	string tstr = ctime(&t);
	tstr[tstr.length()-1] = 0; // chop off last "\n"
	lServer->SetText(TString(servers_str));
	lReceived->SetText(TString(tstr));
	lHistogram->SetText(TString(hnamepath));
	
	// Trigger resize of controls widgets if number of
	// lines changed.
	int Nlines = 1;
	for(uint32_t i=0; i<servers_str.length(); i++) if(servers_str[i]=='\n') Nlines++;
	if(Nlines != last_servers_str_Nlines){
		fTabMainLeft->Resize();
		last_servers_str_Nlines = Nlines;
	}

	// Draw the histogram/macro
	canvas->cd();
	map<string,hdef_t>::iterator hdef_it;
	switch(type){
		case hdef_t::oneD:
		case hdef_t::twoD:
		case hdef_t::threeD:
		case hdef_t::profile:
			if(sum_hist){
				if(sum_hist_modified > currently_displayed_modified){

					// Draw histogram
					sum_hist->Draw();					
				}
			}
			break;
		case hdef_t::macro:
			RS_INFO->Lock();
			hdef_it = RS_INFO->histdefs.find(hnamepath);
			if(hdef_it != RS_INFO->histdefs.end()) RSMF->DrawMacro(canvas, hdef_it->second);
			RS_INFO->Unlock();
			break;
		default:
			cout << "histogram/macro not available (is producer program running?)" << endl;
		
	}

	canvas->Update();
	
	// Release ROOT
	pthread_rwlock_unlock(ROOT_MUTEX);
	
	last_update = now;
}

//----------
// DoUpdateWithFollowUp
//----------
void RSTab::DoUpdateWithFollowUp(void)
{
	/// This is generally called when the selection of histogram has changed
	/// and needs to be requested and then updated on the screen. Since there
	/// is some delay between the requests going out and the results coming
	/// back, DoUpdate can't immediately update the screen in one call so multiple
	/// calls to it are needed with some delay in between. This will call it 
	/// once immediately(-ish), but then schedule two other calls to it for 250ms
	/// and 1s later. That should give enough time for a response to come back.

	// NOTE: Calling DoUpdate() directly here was resulting in se.g faults on
	// OS X in the rs_cmg::RegisterHistogram() method (??!!) Register for ROOT
	// to call it in 10ms 
	TTimer::SingleShot(10, "RSTab", this, "DoUpdate()");
	TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
	TTimer::SingleShot(1000, "RSTab", this, "DoUpdate()");
}

//----------
// DoSelectHists
//----------
void RSTab::DoSelectHists(void)
{
	new Dialog_SelectHists(this, gClient->GetRoot(), 10, 10);
}


