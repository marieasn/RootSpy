
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
using namespace std;

#include <DRootSpy.h>

#include <TH2.h>
#include <TRandom.h>
#include <TMath.h>
#include <TExec.h>
#include <TCanvas.h>

bool DONE = false;

void sigHandler(int sig) { DONE = true; }

int main(int narg, char *argv[])
{
	DRootSpy* RS = new DRootSpy();

	signal(SIGINT, sigHandler);

	// make a little display
	TCanvas *c1 = new TCanvas("c1", "", 600, 600);

	// Lock access to ROOT global while we access it
	pthread_rwlock_wrlock(gROOTSPY_RW_LOCK);

	// Random number generator
	TRandom ran;
			
	////////////////////////////////////
	// build macro
	string macro = "";
	macro += "{\n";
	macro += "// Draw axes\n";
	//macro += "TH2D *axes = new TH2D(\"axes\", \"CDC Occupancy 2\", 100, -65.0, 65.0, 100, -65.0, 65.0);\n";
	macro += "TH2D *axes = (TH2D *)gDirectory->Get(\"axes\");\n";
	macro += "if(!axes)\n";
	macro += "axes = new TH2D(\"axes\", \"CDC Occupancy 2\", 100, -65.0, 65.0, 100, -65.0, 65.0);\n";
	macro += "\n";
	macro += "axes->SetStats(0);\n";
	macro += "axes->Draw();\n";
	macro += "\n";
	macro += "// Overlay all ring histos\n";
	macro += "hist0 = gDirectory->Get(\"ring01\");\n"; 
	macro += "hist0->Draw(\"colz pol same\"); // draw first histo with 'colz' so palette is drawn\n";
	macro += "hist0 = gDirectory->Get(\"ring01\");\n"; 
	macro += "hist0->Draw(\"colz pol same\"); // draw first histo with 'colz' so palette is drawn\n";
	macro += "\n";
	macro += "for(unsigned int iring=1; iring<28; iring++){\n";
	macro += "char hname[256];\n";
	macro += "sprintf(hname, \"ring%02d\", iring+1);\n";
	macro += "TH1 *h = (TH1*)(gDirectory->Get(hname));\n";
	macro += "h->Draw(\"col pol same\"); // draw remaining histos without overwriting color palette\n";
	macro += "}\n";
	macro += "}\n";

	// register it!
	RS->RegisterMacro("CDCwires", "/", macro);
	RS->SetMacroVersion("CDCwires", "/", 1);
	////////////////////////////////////

	////////////////////////////////////
	// create histograms
	////////////////////////////////////
	int Nstraws[28] = {42, 42, 54, 54, 66, 66, 80, 80, 93, 93, 106, 106, 123, 123, 135, 135, 146, 146, 158, 158, 170, 170, 182, 182, 197, 197, 209, 209};
	double radius[28] = {10.72134, 12.08024, 13.7795, 15.14602, 18.71726, 20.2438, 22.01672, 23.50008, 25.15616, 26.61158, 28.33624, 29.77388, 31.3817, 32.75838, 34.43478, 35.81146, 38.28542, 39.7002, 41.31564, 42.73042, 44.34078, 45.75302, 47.36084, 48.77054, 50.37582, 51.76012, 53.36286, 54.74716};
	double phi[28] = {0, 0.074707844, 0.038166294, 0.096247609, 0.05966371, 0.012001551, 0.040721951, 0.001334527, 0.014963808, 0.048683644, 0.002092645, 0.031681749, 0.040719354, 0.015197341, 0.006786058, 0.030005892, 0.019704045, -0.001782064, -0.001306618, 0.018592421, 0.003686784, 0.022132975, 0.019600866, 0.002343723, 0.021301449, 0.005348855, 0.005997358, 0.021018761}; 
	// Define a different 2D histogram for each ring. X-axis is phi, Y-axis is radius (to plot correctly with "pol" option)
	vector<TH2D*> hist;
	for(int iring=0; iring<28; iring++){
		double r_start = radius[iring] - 0.8;
		double r_end = radius[iring] + 0.8;
		double phi_start = phi[iring]; // this is for center of straw. Need additional calculation for phi at end plate
		double phi_end = phi_start + TMath::TwoPi();
		
		char hname[256];
		sprintf(hname, "ring%02d", iring+1);
		
		TH2D *h = new TH2D(hname, "", Nstraws[iring], phi_start, phi_end, 1, r_start, r_end);
		hist.push_back(h);
		RS->AddMacroHistogram("CDCwires", "/", h);
	}
	 
	// Set or fill bin contents using the following where ring=1-28 and straw=1-N :
	//   hist[ring-1]->SetBinContent(straw, 1, w);
	//           or
	//   hist[ring-1]->Fill(straw, 1);
	for(int iring=1; iring<=28; iring++){
		for(int istraw=1; istraw<=Nstraws[iring-1]; istraw++){
			double w = (double)istraw+100.0*(double)iring; // value to set (just for demonstration)
			hist[iring-1]->SetBinContent(istraw, 1, w);
		}
	}
	////////////////////////////////////

	// Release lock on ROOT global
	pthread_rwlock_unlock(gROOTSPY_RW_LOCK);

	// list current histograms
	gDirectory->ls();

	// write the macro to a file
	char filename[] = "/tmp/RootSpy.macro.XXXXXX";
	int macro_fd = mkstemp(filename);
	if (macro_fd != -1) {
		// write the macro to the file
		write(macro_fd, macro.c_str(), macro.length());
		close(macro_fd);

		// try the macro out
		TExec *exec_shell = new TExec();
		string cmd = string(".x ") + filename;
		exec_shell->Exec(cmd.c_str());
	}


#if 0
	//// Plot the test histogram
	// Draw axes
	TH2D *axes = new TH2D("axes", "CDC Occupancy", 100, -65.0, 65.0, 100, -65.0, 65.0);
	axes->SetStats(0);
	axes->Draw();
	
	// Overlay all ring histos
	hist[0]->Draw("colz pol same"); // draw first histo with 'colz' so palette is drawn
	
	for(unsigned int iring=1; iring<hist.size(); iring++){
		char hname[256];
		sprintf(hname, "ring%02d", iring+1);
		hist[iring-1]->Draw("col pol same"); // draw remaining histos without overwriting color palette
	}
#endif

	// print out whatever we drew
	// for some reason the canvas isn't popping out a new window??
	c1->Print("out.pdf");

	while (!DONE) {
		// wait forever...
		sleep(5);
	}


	cout << endl;
	cout << "Ending" << endl;

	// clean up macro file
	unlink(filename);

	return 0;
}


