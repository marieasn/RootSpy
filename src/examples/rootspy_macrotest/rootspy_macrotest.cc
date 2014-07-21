
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <cmath>
using namespace std;

#include <DRootSpy.h>

#include <TH2.h>
#include <TRandom.h>
#include <TMath.h>

bool DONE = false;

void sigHandler(int sig) { DONE = true; }

int main(int narg, char *argv[])
{
	DRootSpy* RS = new DRootSpy();

	signal(SIGINT, sigHandler);
	
	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	// Random number generator
	TRandom ran;
			
	////////////////////////////////////
	// build macro
	string script = "";
	script += "// Draw axes\n";
	script += "TH2D *axes = new TH2D(\"axes\", \"CDC Occupancy\", 100, -65.0, 65.0, 100, -65.0, 65.0);\n";
	script += "axes->SetStats(0);\n";
	script += "axes->Draw();\n";
	script += "\n";
	script += "// Overlay all ring histos\n";
	script += "hist[0]->Draw(\"colz pol same\"); // draw first histo with 'colz' so palette is drawn\n";
	script += "\n";
	script += "for(unsigned int i=1; i<hist.size(); i++){\n";
	script += "char hname[256];\n";
	script += "sprintf(hname, \"ring%02d\", iring+1);\n";
	script += "h = ()gDirectory->Get(hname);\n";
	script += "h->Draw(\"col pol same\"); // draw remaining histos without overwriting color palette\n";
	script += "}\n";

	// register it!
	RS->RegisterMacro("CDCwires", "/", script);
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
		RS->AddMacroHistogram("CDCwires", "/", (TH1*)h);
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
	pthread_mutex_unlock(gROOTSPY_MUTEX);
	
	while (!DONE) {
		// wait forever...
		sleep(5);
	}


	cout << endl;
	cout << "Ending" << endl;

	return 0;
}


