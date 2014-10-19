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

#include "TGaxis.h"


//---------------------------------
// DrawOverlay  (global)
//   this function is exposed for use in macros
//---------------------------------
void DrawOverlay(void) {
	RSMF->current_tab->DoOverlay();  
}

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
	canvas->SetDoubleBuffer();           // for smoother updates
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

	//// For production use, default to not showing anything to start out with

	/*
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
	*/
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

/**
	// If this wasn't in the current list, then add it to our list!
	hnamepaths.push_back(hnamepath);
	currently_displayed = 0;
	TTimer::SingleShot(1, "RSTab", this, "DoUpdate()"); // have canvas updated quickly
	TTimer::SingleShot(250, "RSTab", this, "DoUpdate()");
**/
	return;
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
	if((uint32_t)currently_displayed >= hnamepaths.size()) currently_displayed = 0;
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
	if((uint32_t)currently_displayed >= hnamepaths.size()) currently_displayed = 0;

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
	map<string,hdef_t>::iterator hdef_it;
	switch(type){
		case hdef_t::oneD:
		case hdef_t::twoD:
		case hdef_t::threeD:
		case hdef_t::profile:
			if(sum_hist){
				if(sum_hist_modified > currently_displayed_modified){

					// Draw histogram
					pthread_rwlock_wrlock(ROOT_MUTEX);
					canvas->cd();
					if(type == hdef_t::twoD)
						sum_hist->Draw("COLZ");
					else
						sum_hist->Draw();
					DoOverlay();
					sum_hist->UseCurrentStyle();   // updates in case the style paramteres change
					canvas->Update();
					pthread_rwlock_unlock(ROOT_MUTEX);				
				}
			}
			break;
		case hdef_t::macro:
			canvas->cd();
			canvas->Clear();
			currently_displayed_modified = now;
			RS_INFO->Lock();
			hdef_it = RS_INFO->histdefs.find(hnamepath);
			if(hdef_it != RS_INFO->histdefs.end()) RSMF->DrawMacro(canvas, hdef_it->second);
			RS_INFO->Unlock();
			canvas->Update();
			RequestUpdatedMacroHists();
			break;
		default:
			cout << "histogram/macro not available (is producer program running?)" << endl;
		
	}

	last_update = now;
}


//----------
// DoOverlay
// NOTE: Currently only works for summed histograms
//----------
void RSTab::DoOverlay(void)
{
	//cerr << "In DoOverlay()..." << endl;
	// reset some display defaults that might get changed
	gStyle->SetStatX(0.95);
	gPad->SetTicks();

	// check to see if we should be overlaying archived histograms
	bool overlay_enabled = (RSMF->bShowOverlays->GetState()==kButtonDown);
	if(!overlay_enabled)
		return;
	if(RSMF->archive_file == NULL)
		return;
	
	// make sure that there is an active pad
	if(!gPad)
		return;

	//cerr << "Searching gPad..." << endl;

	// look for the histograms in the current pad
	TIter nextObj(gPad->GetListOfPrimitives());
	TObject *obj;
	TH1 *h = NULL;
	while ((obj = nextObj())) {
		//obj->Draw(next.GetOption());
		TNamed *namedObj = dynamic_cast<TNamed*>(obj);
		if(namedObj != NULL) {
			//cerr << namedObj->GetName() << endl;
			h = static_cast<TH1*>(namedObj);
			break;
		}

		// check to make sure this is actually a histogram
		//h = dynamic_cast<TH1*>(h);
		//if(h!=NULL)
		//	break;
	}
	
	if(h == NULL) {
		cerr << "Could not find histogram info in gPad!" << endl;
		return;
	}

	double hist_ymax = 1.1*h->GetMaximum();
	double hist_ymin = h->GetMinimum();
	
	// find the histogram in our list of histogram defintions
	// so that we can find its full path
	RS_INFO->Lock();
	
	string hnamepath = "";
	hdef_t::histdimension_t type = hdef_t::noneD;
	for(map<string,hdef_t>::iterator hdef_itr = RS_INFO->histdefs.begin();
	    hdef_itr != RS_INFO->histdefs.end(); hdef_itr++) {
		if( h == hdef_itr->second.sum_hist ) {
			hnamepath = hdef_itr->first;
			type = hdef_itr->second.type;
			break;
		}
	}

	RS_INFO->Unlock();
	
	if(hnamepath == "") {
		cerr << "Could not find histogram information in hdef list!" << endl;
		return;
	} 
	//else {
	//	cerr << "Found info for histogram = " << hnamepath << endl;
	//}
	
	
	// overlaying only makes sense for 1D histograms
	if(type != hdef_t::oneD)
		return;
	
	// try to find a corresponding one in the archived file
	// we assume that the archived file has the same hnamepath structure
	// as the histograms in memory
	
	TH1 *h_over_orig = static_cast<TH1*>(RSMF->archive_file->Get(hnamepath.c_str()));
	TH1 *h_over = static_cast<TH1*>(h_over_orig->Clone());   // memory leak??
        if(h_over == NULL) { 
		cerr << "Could not find corresponding archived histogram!" << endl;
		return;
	}	

        // format overlay histogram and extract parameters
	gStyle->SetStatX(0.85); 
	gPad->SetTicky(0);
        h_over->SetStats(0);
        h_over->SetLineWidth(3);
        h_over->SetLineColor(4);
        double overlay_ymin = h_over->GetMinimum(); 
        double overlay_ymax = 1.1*h_over->GetMaximum();

        // Draw the archived histogram overlayed on the current histogram
        //h_over->Scale( h->Integral() / h_over->Integral() );
        float scale = hist_ymax/overlay_ymax;
        h_over->Scale( scale );
        h_over->Draw("SAME");

	// Add in an axis for the overlaid histogram on the right side of the plot
        TGaxis *overlay_yaxis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),
				   gPad->GetUxmax(),gPad->GetUymax(),
				   overlay_ymin,overlay_ymax,505,"+L");
        overlay_yaxis->SetLabelFont( h->GetLabelFont() );                                                                                                                                          
        overlay_yaxis->SetLabelSize( h->GetLabelSize() );                                                                                                                                          
        overlay_yaxis->Draw();                                                                                                                                                                        

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

//----------
// RequestUpdatedMacroHists
//----------
void RSTab::RequestUpdatedMacroHists(void)
{
	/// Request updated histograms based on what is currently displayed in
	/// the canvas. This used when a macro has been used to draw on the canvas
	/// and we need to get updated histograms from the servers. It looks through 
	/// all of the pads and finds all TH1 objects associated with them. If the
	/// TH1 is located in the sum hist directory ("RootSpy:/rootspy/") then a request
	/// is sent out to all servers with the appropriate namepath.

	for(int ipad=0; ipad<100; ipad++){
		TVirtualPad *pad = canvas->GetPad(ipad);
		if(!pad) break;
		
		pad->cd();
		TIter next(pad->GetListOfPrimitives());
		TObject *obj;
		while ( (obj = next()) ){
			TH1 *h = dynamic_cast<TH1*>(obj);
			if(h != NULL){
				TDirectory *dir = h->GetDirectory();
				string path = dir->GetPath();
				string prefix = "RootSpy:/rootspy/";
				if(path.find(prefix) == 0){
					string hnamepath = path.substr(prefix.length()-1) + "/" + h->GetName();
					RS_INFO->RequestHistograms(hnamepath);
				}
			}			
		}
	}
}


