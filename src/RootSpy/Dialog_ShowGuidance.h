// $Id: Dialog_ShowGuidance.h $
// $HeadURL: https://halldsvn.jlab.org/repos/trunk/online/packages/RootSpy/src/RootSpy/Dialog_ShowGuidance.h $
//
//    File: Dialog_ShowGuidance.h
// Creator: davidl


#ifndef _Dialog_ShowGuidance_
#define _Dialog_ShowGuidance_

#include <TGClient.h>
#include <TGButton.h>
#include <TGListBox.h>

using namespace std;

class Dialog_ShowGuidance:public TGMainFrame{
	public:
		Dialog_ShowGuidance(string title, string macro_str);
		virtual ~Dialog_ShowGuidance();
		
		void CloseWindow(void);

	private:
		ClassDef(Dialog_ShowGuidance,1)


};

#endif // _Dialog_ShowGuidance_

