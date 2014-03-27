
#include <unistd.h>
#include <iostream>
#include <cmath>
using namespace std;

#include <DRootSpy.h>


#include <TH1.h>
#include <TRandom.h>
#include <TTree.h>


int main(int narg, char *argv[])
{
	new DRootSpy();

	
	// Define some histograms to file

	TDirectory *main = gDirectory;
	gDirectory->mkdir("components")->cd();

	TH1D *h_px = new TH1D("px", "Momentum X-component", 500, 0.0, 10.0);
	TH1D *h_py = new TH1D("py", "Momentum Y-component", 500, 0.0, 10.0);
	TH1D *h_pz = new TH1D("pz", "Momentum Z-component", 500, 0.0, 10.0);

	main->cd();

	//gDirectory->cd("..");
	TH1D *h_E = new TH1D("E", "Energy", 500, 0.0, 10.0);
	TH1D *h_Mass = new TH1D("Mass", "Mass", 1000, 0.0, 2.0);
	
	// Define some local variables


	double px,py,pz,E,Mass;

	// Define some trees to save
	TTree *T = new TTree("T", "Event Info");
	T->Branch("px", &px, "px/D");
	T->Branch("py", &py, "py/D");
	T->Branch("pz", &pz, "pz/D");
	T->Branch("E", &E, "E/D");
	T->Branch("Mass", &Mass, "Mass/D");

	// Set nice labels for X-axes
	h_px->SetXTitle("p_{x} (GeV/c)");
	h_py->SetXTitle("p_{y} (GeV/c)");
	h_pz->SetXTitle("p_{z} (GeV/c)");
	h_E->SetXTitle("Energy (GeV)");
	h_Mass->SetXTitle("mass (GeV/c^2)");
	
	// Random number generator
	TRandom ran;
	
	// Loop forever while filling the hists
	cout<<endl<<"Filling histograms ..."<<endl;
	unsigned int Nevents=0;
	while(true){
		
		// Randomly choose values to put into an x and y coordinate on some plane 6 meters away from
		double x = (ran.Rndm()-0.5)*2.0;
		double y = (ran.Rndm()-0.5)*2.0;
		double z = 6.0;
		double p = ran.Landau(0.5, 0.075); // total momentum
		double m = 0.134;
		double Etot = sqrt(p*p + m*m) + ran.Gaus(0.0, 0.005);

		//px->Fill(p*x/z);
		//py->Fill(p*y/z);
		//pz->Fill(sqrt(p*p*(1.0 - (x*x+y*y)/(z*z))));
		//E->Fill(Etot);
		//Mass->Fill(sqrt(Etot*Etot - p*p));
		
		px = p*x/z;
		py = p*y/z;
		pz = sqrt(p*p*(1.0 - (x*x+y*y)/(z*z)));
		E = Etot;
		Mass = sqrt(Etot*Etot - p*p);

		h_px->Fill(px);
		h_py->Fill(py);
		h_pz->Fill(pz);
		h_E->Fill(E);
		h_Mass->Fill(Mass);
		
		if(((++Nevents) % 100)==0){
		  //gDirectory->ls();
		  
		  cout <<" "<<Nevents<<" events simulated "<<endl;
		  
		  //cout<<" "<<Nevents<<" events simulated "<<"\r";
		  //cout.flush();
		}


		// sleep a bit in order to limit how fast the histos are filled
		usleep(5000);
	}

	return 0;
}


