// $Id$

//    File: Dialog_AskReset.h
// Creator: sdobbs


#ifndef _Dialog_AskReset_
#define _Dialog_AskReset_

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

#ifndef __CINT__
#include "rs_info.h"
#endif //__CINT__

using namespace std;

class Dialog_AskReset:public TGMainFrame{
	public:
		Dialog_AskReset(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~Dialog_AskReset();
		
		
		void DoOK(void);
		void DoCancel(void);

		void CloseWindow(void);

	private:

		void CreateGUI(void);
		ClassDef(Dialog_AskReset,1)
};

#endif // _Dialog_AskReset_

