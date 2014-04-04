
#include <unistd.h>
#include <iostream>
#include <cmath>
using namespace std;

#include <DRootSpy.h>


#include <TH1.h>
#include <TRandom.h>
#include <TTree.h>
#include <TFile.h>
#include <TROOT.h>
#include <TList.h>
#include <TKey.h>



int main(int narg, char *argv[])
{
	if(narg < 2){
		cout << endl;
		cout << "  Usage:" << endl;
		cout << "     rootspy_filepub file.root [hist_or_tree] [hist_or_tree] ..." << endl;
		cout << endl;
		cout << "Open the given root file and pubilish its contents" << endl;
		cout << "to the RootSpy system specified by the ROOTSPY_UDL" << endl;
		cout << "environment variable. If only the filename is given" << endl;
		cout << "then all histograms and trees will be read into memory" << endl;
		cout << "making them available. If one or more additional arguments" << endl;
		cout << "are given then only histograms or trees of those names" << endl;
		cout << "are read." << endl;
		cout << endl;
		return -1;
	}

	// Open file
	TFile *f = new TFile(argv[1]);
	if( ! f->IsOpen() ){
		cout << "Unable to open ROOT file: " << argv[1] << endl;
		return -2;
	}

	// Read in some or all histograms and TTrees into memory
	TIter iter(f->GetListOfKeys());
	TKey *key;
	while( (key = (TKey*)iter()) ) {
		string name(key->GetName());

		// Optionally read in this object if the user specified
		// additional arguments on the command line AND this
		// matches one of them.
		bool read_obj = true;
		if(narg>2){
			read_obj = false;
			for(int i=2; i<narg; i++){
				if(string(argv[i]) == name) read_obj = true;
			}
		}

		if(!read_obj) continue;

		cout << "Reading: " << name << endl; 
		key->ReadObj();
	}

	// Create DRootSpy object
	new DRootSpy();

	// Sleep forever
	cout << "Hit ctl-C to exit ...." << endl;
	while(true) sleep(1);
	
	f->Close();

	return 0;
}


