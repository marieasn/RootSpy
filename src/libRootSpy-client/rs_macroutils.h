// $Id$
//
//    File: rs_macroutils.h
// Created: Sat Oct  7 10:14:30 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 Darwin Kernel Version 15.6.0: Tue Apr 11 16:00:51 PDT 2017; root:xnu-3248.60.11.5.3~1/RELEASE_X86_64 x86_64)

#include <string>
using std::string;

void rs_SetFlag(const string flag, int val);
int  rs_GetFlag(const string flag);
void rs_ResetHisto(const string hnamepath);
void rs_RestoreHisto(const string hnamepath);

