// $Id$

//    File: Dialog_ScaleOpts.h
// Creator: sdobbs


#ifndef _Dialog_ScaleOpts_
#define _Dialog_ScaleOpts_

#include <TGClient.h>
#include <TTimer.h>
#include <TGComboBox.h>
#include <TGButton.h>
#include <TGListBox.h>
#include <TGPicture.h>
#include <TGTextEntry.h>
#include <TVirtualStreamerInfo.h>
#include <vector>
#include <map>
#include <TGFileDialog.h>
#include <TGButton.h>
#include <TGButtonGroup.h>

#ifndef __CINT__
#include "rs_info.h"
#endif //__CINT__
using namespace std;

class Dialog_ScaleOpts:public TGMainFrame{
	public:
		Dialog_ScaleOpts(string hnamepath, const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_ScaleOpts();
		
		
		void DoOK(void);
		void DoCancel(void);
		void DoRadioButton(Int_t id);

		void CloseWindow(void);

	private:
	
		string hnamepath;
		int Nbins;

		TGButtonGroup *group;

		TGRadioButton *button1;
		TGRadioButton *button2;
		TGRadioButton *button3;
		TGRadioButton *button4;

		TGTextEntry *minBox;
		TGTextEntry *maxBox;

		void CreateGUI(void);
		ClassDef(Dialog_ScaleOpts,1)


#ifndef __CINT__
		// CINT doesn't need to know about the innerds of the class, just the GUI elements
		map<string,hdef_t>::iterator current_hdef_iter;
		int selected_mode;
		    
#endif //__CINT__

};

#endif // _Dialog_ScaleOpts_

