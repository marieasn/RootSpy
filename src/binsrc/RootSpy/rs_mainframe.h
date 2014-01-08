
#ifndef _RS_MAINFRAME_H_
#define _RS_MAINFRAME_H_


// This class is made into a ROOT dictionary ala rootcint.
// Therefore, do NOT include anything Hall-D specific here.
// It is OK to do that in the .cc file, just not here in the 
// header.

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <time.h>

#include <TGClient.h>
#include <TGButton.h>
#include <TCanvas.h>
#include <TText.h>
#include <TRootEmbeddedCanvas.h>
#include <TTUBE.h>
#include <TNode.h>
#include <TGComboBox.h>
#include <TPolyLine.h>
#include <TEllipse.h>
#include <TMarker.h>
#include <TVector3.h>
#include <TGLabel.h>
#include <TTimer.h>
#include <TH1.h>
#include <TFile.h>
#include <TGMenu.h>
#include <TGDockableFrame.h>

#include "Dialog_SelectHists.h"
#include "Dialog_SaveHists.h"
#include "hinfo_t.h"
#include "hdef_t.h"

class rs_mainframe:public TGMainFrame {

	public:
		rs_mainframe(const TGWindow *p, UInt_t w, UInt_t h);
		~rs_mainframe(){};
		
		enum viewStyle_t_rs{
			kViewByObject_rs,
			kViewByServer_rs
		};

		

		void ReadPreferences(void);
		void SavePreferences(void);
		
		// Slots for ROOT GUI
		void DoQuit(void);
		void MakeTB(void);
		void DoTimer(void);
//		void DoSelectServerHist(void);
		void DoSelectHists(void);
		//void DoSetVBServer(void);
		//void DoSetVBObject(void);
		//void DoRadio(void);
		void DoSelectDelay(Int_t index);
		void DoUpdate(void);
// needs to change back to DoNext(void)
		void DoNext(void);
		void DoPrev(void);
		void DoLoopOverServers(void);
		void DoLoopOverHists(void);
		void DoSave(void);
		void DoSaveHists(void);
		void DoFinal(void);
		void DoIndiv(void);
		void CloseWindow(void);
		void DoTreeInfo(void);
		void DoTreeInfoShort(void);
		void DoOnline(void);
		void DoSetArchiveFile(void);
		void DoLoadHistsList(void);
		void DoSaveHistsList(void);

		void HandleMenu(Int_t id);
		Bool_t HandleKey(Event_t *event);
		
		TGMainFrame *dialog_selectserverhist;
		TGMainFrame *dialog_selecthists;
		TGMainFrame *dialog_savehists;
		TGMainFrame *dialog_indivhists;
		TGMainFrame *dialog_selecttree;
		TGLabel *selected_server;
		TGLabel *selected_hist;
		TGLabel *retrieved_lab;
		TGComboBox *delay;
		TCanvas *canvas;
		TGCheckButton *auto_refresh;
		TGCheckButton *loop_over_servers;
		TGCheckButton *loop_over_hists;
		TGCheckButton *show_archived_hists;
		TGTextButton *indiv;

		TGDockableFrame    *fMenuDock;
		TGMenuBar  *fMenuBar;
		TGPopupMenu  *fMenuFile, *fMenuTools;
		TGLayoutHints      *fMenuBarLayout, *fMenuBarItemLayout, *fMenuBarHelpLayout;

		bool delete_dialog_selectserverhist;
		bool delete_dialog_selecttree;
		bool delete_dialog_selecthists;
		bool delete_dialog_savehists;
		bool delete_dialog_indivhists;
		bool can_view_indiv;

	protected:
			viewStyle_t_rs viewStyle_rs;
			void DrawHist(TH1 *hist, string hnamepath,
				      hdef_t::histdimension_t hdim );
	private:
	
		TTimer *timer;
		long sleep_time; // in milliseconds
		time_t last_called;				// last time DoTimer() was called
		time_t last_ping_time;			// last time we broadcast for a server list
		time_t last_hist_requested;	// last time we requested a histogram (see rs_info for time we actually got it)
		
		
		TGRadioButton *VBServer;

		time_t delay_time;
		
		hid_t last_requested;	// last hnamepath requested
		TH1 *last_hist_plotted;
		void CreateGUI(void);

		// info for comparing with archived histograms
		//bool overlay_mode; // maybe don't need this?
		TFile *archive_file;
		
	ClassDef(rs_mainframe,1)
};



#endif //_RS_MAINFRAME_H_
