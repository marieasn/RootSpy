// $Id$
//
//    File: rs_macroutils.cc
// Created: Sat Oct  7 10:14:30 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 Darwin Kernel Version 15.6.0: Tue Apr 11 16:00:51 PDT 2017; root:xnu-3248.60.11.5.3~1/RELEASE_X86_64 x86_64)

// This file contains some global scope routines that are made
// available to macros. They allow the macros to interact with
// the RootSpy client in various ways that extend their functionality.

#include <iostream>
#include <map>
#include <thread>
using namespace std;

#include "rs_info.h"
#include "rs_macroutils.h"

static map<string, int> rs_flags;

std::set<string> rs_CheckAgainstAI_fnames;

//-------------------
// rs_SetFlag
//
// Set a named flag that can be accessed by macros
//-------------------
void rs_SetFlag(const string flag, int val)
{
	rs_flags[flag] = val;
}

//-------------------
// rs_GetFlag
//
// Get a named flag and return it as an int. 
//-------------------
int rs_GetFlag(const string flag)
{
	if(rs_flags.count(flag)){
		return rs_flags[flag];
	}else{
		cerr << "Unknown flag \"" << flag << "\" requested from macro!" << endl;
		return -1;
	}
}

//-------------------
// rs_ResetHisto
//
// Reset a histogram on the rootspy client. This
// is equivalent to pushing the "Reset" button
// on the RootSpy GUI program except this will
// only reset a single histogram. The histogram
// can be restored using re_RestoreHisto.
//-------------------
void rs_ResetHisto(const string hnamepath)
{
	cout << "Resetting: " << hnamepath << endl;
	
	// We need to do this in a separate thread since this
	// will be called from a macro which will already have
	// mutexes locked.
	thread t( [hnamepath](){RS_INFO->ResetHisto(hnamepath);} );
	t.detach();
}

//-------------------
// rs_RestoreHisto
//
// Restore a histogram on the rootspy client. This
// is equivalent to pushing the "Restore" button
// on the RootSpy GUI program except this will
// only restore a single histogram. This reverses
// a previous call to re_ResetHisto.
//-------------------
void rs_RestoreHisto(const string hnamepath)
{
	cout << "Restoring: " << hnamepath << endl;

	// We need to do this in a separate thread since this
	// will be called from a macro which will already have
	// mutexes locked.
	thread t( [hnamepath](){RS_INFO->RestoreHisto(hnamepath);} );
	t.detach();
}

//-------------------
// rs_ResetAllMacroHistos
//
// Reset all histograms associated with the specified
// macro. This is equivalent to pushing the "Reset"
// button on the RootSpy GUI program for each histogram
// a macro has specified in its comments. The histograms
// can be restored using re_RestoreAllMacroHistos.
//-------------------
void rs_ResetAllMacroHistos(const string hnamepath)
{
	if( RS_INFO->histdefs.count(hnamepath) ==0 ){
		_DBG_<< " rs_ResetAllMacroHistos called with hnamepath==" << hnamepath << " but no such hdef_t exists!" << endl;
		return;
	}

	auto &hdef = RS_INFO->histdefs[hnamepath];
	auto macro_hnamepaths = hdef.macro_hnamepaths; // make local copy for passing to lambda by value

	cout << "Resetting " << hdef.macro_hnamepaths.size() << " histograms for macro: " << hnamepath << endl;

	// We need to do this in a separate thread since this
	// will be called from a macro which will already have
	// mutexes locked.
	thread t( [hnamepath,macro_hnamepaths](){
		for( auto h : macro_hnamepaths) RS_INFO->ResetHisto(h);
	} );
	t.detach();
}

//-------------------
// rs_RestoreAllMacroHistos
//
// Restore all histograms associated with the specified
// macro. This is equivalent to pushing the "Restore"
// button on the RootSpy GUI program for each histogram
// a macro has specified in its comments.
//-------------------
void rs_RestoreAllMacroHistos(const string hnamepath)
{
	if( RS_INFO->histdefs.count(hnamepath) ==0 ){
		_DBG_<< " rs_RestoreAllMacroHistos called with hnamepath==" << hnamepath << " but no such hdef_t exists!" << endl;
		return;
	}

	auto &hdef = RS_INFO->histdefs[hnamepath];
	auto macro_hnamepaths = hdef.macro_hnamepaths; // make local copy for passing to lambda by value

	cout << "Restoring " << hdef.macro_hnamepaths.size() << " histograms for macro: " << hnamepath << endl;

	// We need to do this in a separate thread since this
	// will be called from a macro which will already have
	// mutexes locked.
	thread t( [hnamepath,macro_hnamepaths](){
		for( auto h : macro_hnamepaths ) RS_INFO->RestoreHisto(h);
	} );
	t.detach();
}

//-------------------
// rs_CheckAgainstAI
//
// This is used by macros to communicate that a specific file is
// ready to be checked using an A.I. model for its status. This
// just adds the given filename to the global rs_CheckAgainstAI_fnames.
// This is used by RSAI program (see that for more details).
//-------------------
void rs_CheckAgainstAI(const string fname)
{
	rs_CheckAgainstAI_fnames.insert( fname );
}

