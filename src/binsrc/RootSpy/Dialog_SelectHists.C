// Mainframe macro generated from application: /usr/local/root/PRO/bin/root.exe
// By ROOT version 5.22/00 on 2009-09-13 17:50:42

#ifndef ROOT_TGDockableFrame
#include "TGDockableFrame.h"
#endif
#ifndef ROOT_TGMenu
#include "TGMenu.h"
#endif
#ifndef ROOT_TGMdiDecorFrame
#include "TGMdiDecorFrame.h"
#endif
#ifndef ROOT_TG3DLine
#include "TG3DLine.h"
#endif
#ifndef ROOT_TGMdiFrame
#include "TGMdiFrame.h"
#endif
#ifndef ROOT_TGMdiMainFrame
#include "TGMdiMainFrame.h"
#endif
#ifndef ROOT_TGuiBldHintsButton
#include "TGuiBldHintsButton.h"
#endif
#ifndef ROOT_TRootBrowserLite
#include "TRootBrowserLite.h"
#endif
#ifndef ROOT_TGMdiMenu
#include "TGMdiMenu.h"
#endif
#ifndef ROOT_TGListBox
#include "TGListBox.h"
#endif
#ifndef ROOT_TGNumberEntry
#include "TGNumberEntry.h"
#endif
#ifndef ROOT_TGScrollBar
#include "TGScrollBar.h"
#endif
#ifndef ROOT_TGuiBldHintsEditor
#include "TGuiBldHintsEditor.h"
#endif
#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif
#ifndef ROOT_TGFileDialog
#include "TGFileDialog.h"
#endif
#ifndef ROOT_TGShutter
#include "TGShutter.h"
#endif
#ifndef ROOT_TGButtonGroup
#include "TGButtonGroup.h"
#endif
#ifndef ROOT_TGCanvas
#include "TGCanvas.h"
#endif
#ifndef ROOT_TGFSContainer
#include "TGFSContainer.h"
#endif
#ifndef ROOT_TGButton
#include "TGButton.h"
#endif
#ifndef ROOT_TGuiBldEditor
#include "TGuiBldEditor.h"
#endif
#ifndef ROOT_TGFSComboBox
#include "TGFSComboBox.h"
#endif
#ifndef ROOT_TGLabel
#include "TGLabel.h"
#endif
#ifndef ROOT_TGMsgBox
#include "TGMsgBox.h"
#endif
#ifndef ROOT_TRootGuiBuilder
#include "TRootGuiBuilder.h"
#endif
#ifndef ROOT_TGTab
#include "TGTab.h"
#endif
#ifndef ROOT_TGListView
#include "TGListView.h"
#endif
#ifndef ROOT_TGSplitter
#include "TGSplitter.h"
#endif
#ifndef ROOT_TGStatusBar
#include "TGStatusBar.h"
#endif
#ifndef ROOT_TGListTree
#include "TGListTree.h"
#endif
#ifndef ROOT_TGToolTip
#include "TGToolTip.h"
#endif
#ifndef ROOT_TGToolBar
#include "TGToolBar.h"
#endif
#ifndef ROOT_TGuiBldDragManager
#include "TGuiBldDragManager.h"
#endif

#include "Riostream.h"

void Dialog_SelectHists()
{

   // main frame
   TGMainFrame *fMainFrame984 = new TGMainFrame(gClient->GetRoot(),10,10,kMainFrame | kVerticalFrame);
   fMainFrame984->SetLayoutBroken(kTRUE);

   // composite frame
   TGCompositeFrame *fMainFrame800 = new TGCompositeFrame(fMainFrame984,400,290,kVerticalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame801 = new TGVerticalFrame(fMainFrame800,396,286,kVerticalFrame);

   // vertical frame
   TGVerticalFrame *fVerticalFrame802 = new TGVerticalFrame(fVerticalFrame801,96,42,kVerticalFrame);
   TGRadioButton *fRadioButton803 = new TGRadioButton(fVerticalFrame802,"View By Object");
   fRadioButton803->SetState(kButtonDown);
   fRadioButton803->SetTextJustify(36);
   fRadioButton803->SetMargins(0,0,0,0);
   fRadioButton803->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton803, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   TGRadioButton *fRadioButton804 = new TGRadioButton(fVerticalFrame802,"View By Server");
   fRadioButton804->SetState(kButtonDown);
   fRadioButton804->SetTextJustify(36);
   fRadioButton804->SetMargins(0,0,0,0);
   fRadioButton804->SetWrapLength(-1);
   fVerticalFrame802->AddFrame(fRadioButton804, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fVerticalFrame801->AddFrame(fVerticalFrame802, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // canvas widget
   TGCanvas *fCanvas805 = new TGCanvas(fVerticalFrame801,384,200);

   // canvas viewport
   TGViewPort *fViewPort806 = fCanvas805->GetViewPort();

   // list tree
   TGListTree *fListTree815 = new TGListTree(fCanvas805,kHorizontalFrame);

   const TGPicture *popen;       //used for list tree items
   const TGPicture *pclose;      //used for list tree items

   TGListTreeItem *item0 = fListTree815->AddItem(NULL,"Entry 1");
   popen = gClient->GetPicture("ofolder_t.xpm");
   pclose = gClient->GetPicture("folder_t.xpm");
   item0->SetPictures(popen, pclose);
   fListTree815->CloseItem(item0);

   fViewPort806->AddFrame(fListTree815);
   fListTree815->SetLayoutManager(new TGHorizontalLayout(fListTree815));
   fListTree815->MapSubwindows();
   fCanvas805->SetContainer(fListTree815);
   fCanvas805->MapSubwindows();
   fVerticalFrame801->AddFrame(fCanvas805, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   // horizontal frame
   TGHorizontalFrame *fHorizontalFrame816 = new TGHorizontalFrame(fVerticalFrame801,392,32,kHorizontalFrame);
   fHorizontalFrame816->SetLayoutBroken(kTRUE);
   TGTextButton *fTextButton817 = new TGTextButton(fHorizontalFrame816,"Cancel");
   fTextButton817->SetTextJustify(36);
   fTextButton817->SetMargins(0,0,0,0);
   fTextButton817->SetWrapLength(-1);
   fTextButton817->Resize(56,22);
   fHorizontalFrame816->AddFrame(fTextButton817, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton817->MoveResize(2,2,56,22);
   TGTextButton *fTextButton818 = new TGTextButton(fHorizontalFrame816,"OK");
   fTextButton818->SetTextJustify(36);
   fTextButton818->SetMargins(0,0,0,0);
   fTextButton818->SetWrapLength(-1);
   fTextButton818->Resize(55,22);
   fHorizontalFrame816->AddFrame(fTextButton818, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));
   fTextButton818->MoveResize(320,0,55,22);

   fVerticalFrame801->AddFrame(fHorizontalFrame816, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fMainFrame800->AddFrame(fVerticalFrame801, new TGLayoutHints(kLHintsLeft | kLHintsTop,2,2,2,2));

   fMainFrame984->AddFrame(fMainFrame800, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   fMainFrame800->MoveResize(0,0,400,290);

   fMainFrame984->SetMWMHints(kMWMDecorAll,
                        kMWMFuncAll,
                        kMWMInputModeless);
   fMainFrame984->MapSubwindows();

   fMainFrame984->Resize(fMainFrame984->GetDefaultSize());
   fMainFrame984->MapWindow();
   fMainFrame984->Resize(490,372);
}  
