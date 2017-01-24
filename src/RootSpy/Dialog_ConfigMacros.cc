// $Id$
//
//    File: Dialog_ConfigMacros.cc
// Creator: sdobbs
//

#include <algorithm>
#include <iostream>
using namespace std;

#include "RootSpy.h"
#include "Dialog_ConfigMacros.h"
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
// Dialog_ConfigMacros    (Constructor)
//---------------------------------
Dialog_ConfigMacros::Dialog_ConfigMacros(const TGWindow *p, UInt_t w, UInt_t h):TGMainFrame(p,w,h, kMainFrame | kVerticalFrame)
{
  cout << "Dialog_ConfigMacros() constructor..." << endl;
	// Define all of the of the graphics objects. 
	CreateGUI();
	//cout<<gClient->GetPicturePool()->GetPath()<<endl;

	listBox->SetMultipleSelections(kFALSE);

	// Finish up and map the window
	SetWindowName("RootSpy Macro Configuration");
	//SetIconName("SelectHist");
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();

}

//---------------------------------
// ~Dialog_ConfigMacros    (Destructor)
//---------------------------------
Dialog_ConfigMacros::~Dialog_ConfigMacros()
{

	cout<<"escaped destructor"<<endl;
}


void Dialog_ConfigMacros::CloseWindow(void) {
  	DeleteWindow();
	RSMF->RaiseWindow();
	RSMF->RequestFocus();
	UnmapWindow();

	RSMF->delete_dialog_configmacros = true;

	// for testing, dump the macro mapping to screen
	_DBG_ << "new macro mapping: " << endl;
	for(map<string,string>::iterator macro_itr = new_macro_files.begin();
	    macro_itr != new_macro_files.end(); macro_itr++) {
	  cout << macro_itr->first << ": " << macro_itr->second << endl;
	}

	// copy over the edited mapping - this may not be threadsafe?
	RSMF->macro_files = new_macro_files;
}


//---------------------------------
// DoAdd
//---------------------------------
void Dialog_ConfigMacros::DoAdd(void)
{	
  // add in new blank entry - don't add it to the mapping unless the info is changed
  listBox->AddEntry(NEW_ENTRY_STR.c_str(), listBox->GetNumberOfEntries()+1);
  listBox->MapSubwindows();
  listBox->Layout();
}

//---------------------------------
// DoDelete
//---------------------------------
void Dialog_ConfigMacros::DoDelete(void)
{
  // if nothing is selected
  if( listBox->GetSelected() < 1 )
    return;

  cerr << "selected entry = " <<  listBox->GetSelected() << endl;

  // delete the entry
  //macro_files.erase();
  TGTextLBEntry *entry = (TGTextLBEntry *)listBox->GetSelectedEntry();
  cout << " removing " << entry->GetText()->GetString() 
       << " (entry #" << listBox->GetSelected() << ")" << endl;

  new_macro_files.erase( entry->GetText()->GetString() );
  listBox->RemoveEntry( listBox->GetSelected() );

}

//---------------------------------
// DoSelectEntry
//---------------------------------
void Dialog_ConfigMacros::DoSelectEntry(Int_t id)
{

  // save current entry, if there is anything in the box currently
  cout << "In DoSelectEntry()..." << endl;
  cout << "currently selected item = " << listBox->GetSelected() << endl;
  TGTextLBEntry *entry = (TGTextLBEntry *)listBox->GetSelectedEntry();
  string selected_hnamepath = entry->GetText()->GetString();

  /**
  string current_hnamepath ( hnamepathBox->GetText() );
  string current_macro ( macroBox->GetText() );

  cout << "  currently in the box = " << current_hnamepath
       << ", " << current_macro << endl;
  cout << "  selected item = " << selected_hnamepath << endl;

  if( ( current_hnamepath != "" ) && ( current_hnamepath != NEW_ENTRY_STR ) )  {
    // see if the name was changed, if so:
    //  - clear out old entry
    //  - update list box title
    cout << " selected hnamepath = " << selected_hnamepath << endl;
    cout << " current hnamepath = " << current_hnamepath << endl;

    if(selected_hnamepath != current_hnamepath) {
      cout << " updating label = " << current_hnamepath.c_str() << endl;
      new_macro_files.erase( selected_hnamepath );
      //entry->GetText()->SetString( current_hnamepath.c_str() );
      entry->SetTitle( current_hnamepath.c_str() );
      listBox->MapSubwindows();
      listBox->Layout();    
    }

    new_macro_files[current_hnamepath] = current_macro;
  }
  **/
  
  //TGTextLBEntry *entry = (TGTextLBEntry *)listBox->GetEntry(id);
  cout << " selected " << entry->GetText()->GetString() << endl;

  // load current information
  if( selected_hnamepath != NEW_ENTRY_STR ) {
    hnamepathBox->SetText( selected_hnamepath.c_str() );
    macroBox->SetText( new_macro_files[selected_hnamepath].c_str() );
  } else {
    hnamepathBox->SetText( NEW_ENTRY_STR.c_str() );
    macroBox->SetText("");
  }
}

//---------------------------------
// DoSaveEntry
//---------------------------------
void Dialog_ConfigMacros::DoSaveEntry()
{

  // save current entry, if there is anything in the box currently
  cout << "In DoSaveEntry()..." << endl;
  cout << "currently selected item = " << listBox->GetSelected() << endl;
  TGTextLBEntry *entry = (TGTextLBEntry *)listBox->GetSelectedEntry();
  string selected_hnamepath = entry->GetText()->GetString();

  string current_hnamepath ( hnamepathBox->GetText() );
  string current_macro ( macroBox->GetText() );

  if( ( current_hnamepath != "" ) && ( current_hnamepath != NEW_ENTRY_STR ) )  {
    // see if the name was changed, if so:
    //  - clear out old entry
    //  - update list box title
    cout << " selected hnamepath = " << selected_hnamepath << endl;
    cout << " current hnamepath = " << current_hnamepath << endl;

    if(selected_hnamepath != current_hnamepath) {
      cout << " updating label = " << current_hnamepath.c_str() << endl;
      new_macro_files.erase( selected_hnamepath );
      //entry->GetText()->SetString( current_hnamepath.c_str() );
      entry->SetTitle( current_hnamepath.c_str() );
      listBox->MapSubwindows();
      listBox->Layout();    
    }

    new_macro_files[current_hnamepath] = current_macro;
  }

}

//---------------------------------
// CreateGUI
//---------------------------------
void Dialog_ConfigMacros::CreateGUI(void)
{
  //cout << "build the dialog GUI!" << endl;

   // main frame
  //TGMainFrame *fMainFrame844 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
  TGMainFrame *fMainFrame844 = this;
  //fMainFrame844->SetName("fMainFrame844");
  //fMainFrame844->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fMainFrame846 = new TGCompositeFrame(fMainFrame844,490,372,kVerticalFrame);
   fMainFrame846->SetName("fMainFrame846");
   fMainFrame846->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fMainFrame1947 = new TGCompositeFrame(fMainFrame846,543,372,kVerticalFrame);
   fMainFrame1947->SetName("fMainFrame1947");
   fMainFrame1947->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fMainFrame1437 = new TGCompositeFrame(fMainFrame1947,541,370,kVerticalFrame);
   fMainFrame1437->SetName("fMainFrame1437");

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame1154 = new TGHorizontalFrame(fMainFrame1437,539,368,kHorizontalFrame);
   fHorizontalFrame1154->SetName("fHorizontalFrame1154");

   TGFont *ufont;         // will reflect user font changes
   //TGFont *ufont_bold;         // will reflect user font changes
   ufont = gClient->GetFont("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
   //ufont_bold = gClient->GetFont("-*-helvetica-bold-r-*-*-12-*-*-*-*-*-iso8859-1");


   // vertical frame
   TGVerticalFrame *fVerticalFrame1161 = new TGVerticalFrame(fHorizontalFrame1154,208,384,kVerticalFrame);
   fVerticalFrame1161->SetName("fVerticalFrame1161");
   TGLabel *fLabel676 = new TGLabel(fVerticalFrame1161,"  Histograms");
   fLabel676->SetTextJustify(36);
   fLabel676->SetMargins(0,0,0,0);
   fLabel676->SetWrapLength(-1);
   fVerticalFrame1161->AddFrame(fLabel676, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // list box
   TGListBox *fListBox1202 = new TGListBox(fVerticalFrame1161);
   fListBox1202->SetName("fListBox1202");
   fListBox1202->Resize(204,308);
   //fListBox1202->AddEntry("test entry", 1);
   fVerticalFrame1161->AddFrame(fListBox1202, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame1250 = new TGHorizontalFrame(fVerticalFrame1161,204,48,kHorizontalFrame);
   fHorizontalFrame1250->SetName("fHorizontalFrame1250");
   fHorizontalFrame1250->SetLayoutBroken(kTRUE);
   TGTextButton *fTextButton685 = new TGTextButton(fHorizontalFrame1250,"Add");
   fTextButton685->SetTextJustify(36);
   fTextButton685->SetMargins(0,0,0,0);
   fTextButton685->SetWrapLength(-1);
   fTextButton685->Resize(97,22);
   fHorizontalFrame1250->AddFrame(fTextButton685, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton685->MoveResize(2,2,97,22);
   TGTextButton *fTextButton686 = new TGTextButton(fHorizontalFrame1250,"Delete");
   fTextButton686->SetTextJustify(36);
   fTextButton686->SetMargins(0,0,0,0);
   fTextButton686->SetWrapLength(-1);
   fTextButton686->Resize(97,22);
   fHorizontalFrame1250->AddFrame(fTextButton686, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton686->MoveResize(103,2,97,22);

   fVerticalFrame1161->AddFrame(fHorizontalFrame1250, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fHorizontalFrame1154->AddFrame(fVerticalFrame1161, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // vertical frame
   TGVerticalFrame *fVerticalFrame1707 = new TGVerticalFrame(fHorizontalFrame1154,323,364,kVerticalFrame);
   fVerticalFrame1707->SetName("fVerticalFrame1707");
   fVerticalFrame1707->SetLayoutBroken(kTRUE);
   TGLabel *fLabel688 = new TGLabel(fVerticalFrame1707,"Histogram Name");
   fLabel688->SetTextJustify(36);
   fLabel688->SetMargins(0,0,0,0);
   fLabel688->SetWrapLength(-1);
   fVerticalFrame1707->AddFrame(fLabel688, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fLabel688->MoveResize(16,30,96,18);


   TGGC   *uGC;           // will reflect user GC changes
   // graphics context changes
   GCValues_t valEntry689;
   valEntry689.fMask = kGCForeground | kGCBackground | kGCFillStyle | kGCFont | kGCGraphicsExposures;
   gClient->GetColorByName("#000000",valEntry689.fForeground);
   gClient->GetColorByName("#e0e0e0",valEntry689.fBackground);
   valEntry689.fFillStyle = kFillSolid;
   valEntry689.fFont = ufont->GetFontHandle();
   valEntry689.fGraphicsExposures = kFALSE;
   uGC = gClient->GetGC(&valEntry689, kTRUE);
   TGTextEntry *fTextEntry689 = new TGTextEntry(fVerticalFrame1707, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
   fTextEntry689->SetMaxLength(4096);
   fTextEntry689->SetAlignment(kTextLeft);
   fTextEntry689->SetText("");
   fTextEntry689->Resize(280,fTextEntry689->GetDefaultHeight());
   fVerticalFrame1707->AddFrame(fTextEntry689, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextEntry689->MoveResize(16,55,280,22);
   TGLabel *fLabel690 = new TGLabel(fVerticalFrame1707,"Macro File Path");
   fLabel690->SetTextJustify(36);
   fLabel690->SetMargins(0,0,0,0);
   fLabel690->SetWrapLength(-1);
   fVerticalFrame1707->AddFrame(fLabel690, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fLabel690->MoveResize(16,100,96,18);

   ufont = gClient->GetFont("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1");

   // graphics context changes
   GCValues_t valEntry691;
   valEntry691.fMask = kGCForeground | kGCBackground | kGCFillStyle | kGCFont | kGCGraphicsExposures;
   gClient->GetColorByName("#000000",valEntry691.fForeground);
   gClient->GetColorByName("#e0e0e0",valEntry691.fBackground);
   valEntry691.fFillStyle = kFillSolid;
   valEntry691.fFont = ufont->GetFontHandle();
   valEntry691.fGraphicsExposures = kFALSE;
   uGC = gClient->GetGC(&valEntry691, kTRUE);
   TGTextEntry *fTextEntry691 = new TGTextEntry(fVerticalFrame1707, new TGTextBuffer(14),-1,uGC->GetGC(),ufont->GetFontStruct(),kSunkenFrame | kDoubleBorder | kOwnBackground);
   fTextEntry691->SetMaxLength(4096);
   fTextEntry691->SetAlignment(kTextLeft);
   fTextEntry691->SetText("");
   fTextEntry691->Resize(280,fTextEntry691->GetDefaultHeight());
   fVerticalFrame1707->AddFrame(fTextEntry691, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextEntry691->MoveResize(16,125,280,22);

   TGTextButton *fTextButton696 = new TGTextButton(fVerticalFrame1707,"Done");
   fTextButton696->SetTextJustify(36);
   fTextButton696->SetMargins(0,0,0,0);
   fTextButton696->SetWrapLength(-1);
   fTextButton696->Resize(60,24);
   fVerticalFrame1707->AddFrame(fTextButton696, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fTextButton696->MoveResize(240,335,60,24);
   fTextButton696->MoveResize(240,325,60,24);

   TGTextButton *fTextButton796 = new TGTextButton(fVerticalFrame1707,"Save");
   fTextButton796->SetTextJustify(36);
   fTextButton796->SetMargins(0,0,0,0);
   fTextButton796->SetWrapLength(-1);
   fTextButton796->Resize(60,24);
   fVerticalFrame1707->AddFrame(fTextButton796, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   //fTextButton796->MoveResize(50,335,60,24);
   fTextButton796->MoveResize(30,325,60,24);

   fHorizontalFrame1154->AddFrame(fVerticalFrame1707, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fMainFrame1437->AddFrame(fHorizontalFrame1154, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,1,1,1,1));

   fMainFrame1947->AddFrame(fMainFrame1437, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fMainFrame1437->MoveResize(0,0,541,370);

   fMainFrame846->AddFrame(fMainFrame1947, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fMainFrame1947->MoveResize(0,0,543,372);

   fMainFrame844->AddFrame(fMainFrame846, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fMainFrame846->MoveResize(0,0,490,372);

   fMainFrame844->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame844->MapSubwindows();

   //fMainFrame844->Resize(fMainFrame844->GetDefaultSize());
   fMainFrame844->MapWindow();
   fMainFrame844->Resize(530,372);
   //==============================================================================================

	// Connect GUI elements to methods
	TGTextButton* &add_button = fTextButton685;
	TGTextButton* &delete_button = fTextButton686;
	TGTextButton* &done_button = fTextButton696;
	TGTextButton* &save_button = fTextButton796;

	this->listBox = fListBox1202;
	this->hnamepathBox = fTextEntry689;
	this->macroBox = fTextEntry691;

	this->listBox->MapSubwindows();
	this->listBox->Layout();    
	
	add_button->Connect("Clicked()","Dialog_ConfigMacros", this, "DoAdd()");
	delete_button->Connect("Clicked()","Dialog_ConfigMacros", this, "DoDelete()");
	save_button->Connect("Clicked()","Dialog_ConfigMacros", this, "DoSaveEntry()");
	done_button->Connect("Clicked()","Dialog_ConfigMacros", this, "CloseWindow()");

	this->listBox->Connect("Selected(Int_t)","Dialog_ConfigMacros", this, "DoSelectEntry(Int_t)");
	//this->listTree->Connect("Checked(TObject*, Bool_t)","Dialog_ConfigMacros", this, "DoCheckedEntry(TObject*, Int_t)");

	// populate the list box
	new_macro_files = RSMF->macro_files;
	int num_entries = 0;
	for(map<string,string>::iterator macro_iter = new_macro_files.begin();
	    macro_iter != new_macro_files.end(); macro_iter++) {
	  this->listBox->AddEntry(macro_iter->first.c_str(), ++num_entries);	  
	}


	
	MapSubwindows();
	Resize(GetDefaultSize());
	MapWindow();
}

