
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <cmath>
using namespace std;

#include <DRootSpy.h>


#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TRandom.h>
#include <TTree.h>
#include <TFile.h>

#define MAX_TREE_EVENTS 10000

bool DONE =false;

void sigHandler(int sig) { DONE = true; }

int main(int narg, char *argv[])
{
	DRootSpy *drootspy = new DRootSpy();

	signal(SIGINT, sigHandler);
	
	// Lock access to ROOT global while we access it
	pthread_mutex_lock(gROOTSPY_MUTEX);

	// Define some histograms to file

	TDirectory *main = gDirectory;
	gDirectory->mkdir("components")->cd();

	TH1D *h_px = new TH1D("px", "Momentum X-component", 500, 0.0, 10.0);
	TH1D *h_py = new TH1D("py", "Momentum Y-component", 500, 0.0, 10.0);
	TH1D *h_pz = new TH1D("pz", "Momentum Z-component", 500, 0.0, 10.0);

	TProfile *h_pvE = new TProfile("pvE", "Total Momentum vs. Energy", 500, 0.0, 10.0);

	main->cd();

	//gDirectory->cd("..");
	TH1D *h_E = new TH1D("E", "Energy", 500, 0.0, 10.0);
	TH1D *h_Mass = new TH1D("Mass", "Mass", 1000, 0.0, 2.0);
	TH2D *h_Mass_E = new TH2D("MassVEnergy", "Energy v. Mass", 1000, 0.0, 2.0, 500, 0.0, 10.0);
	
	// Define some local variables


	double px,py,pz,E,Mass;
	float data[100];
	for(int i=0; i< 100; i++) data[i] = (float)i;

	// Define some trees to save
	TTree *T = new TTree("T", "Event Info");
	T->Branch("px", &px, "px/D");
	T->Branch("py", &py, "py/D");
	T->Branch("pz", &pz, "pz/D");
	T->Branch("E", &E, "E/D");
	T->Branch("Mass", &Mass, "Mass/D");
	T->Branch("data", &data, "data[100]/F");

	// Set nice labels for X-axes
	h_px->SetXTitle("p_{x} (GeV/c)");
	h_py->SetXTitle("p_{y} (GeV/c)");
	h_pz->SetXTitle("p_{z} (GeV/c)");
	h_E->SetXTitle("Energy (GeV)");
	h_Mass->SetXTitle("mass (GeV/c^2)");
	
	// Random number generator
	TRandom ran;
			
	// Release lock on ROOT global
	pthread_mutex_unlock(gROOTSPY_MUTEX);
	
	// Loop forever while filling the hists
	cout<<endl<<"Filling histograms ..."<<endl;
	unsigned int Nevents=0;
	while(!DONE){
		
		// Randomly choose values to put into an x and y coordinate on some plane 6 meters away from
		//double x = (ran.Rndm()-0.5)*2.0;
		//double y = (ran.Rndm()-0.5)*2.0;
		double x = (ran.Rndm()-0.5)*3.0;
		double y = (ran.Rndm()-0.5)*3.0;
		double z = 6.0;
		double p = ran.Landau(0.5, 0.075); // total momentum
		double m = 0.134;
		double Etot = sqrt(p*p + m*m) + ran.Gaus(0.0, 0.005);
		
		px = p*x/z;
		py = p*y/z;
		pz = sqrt(p*p*(1.0 - (x*x+y*y)/(z*z)));
		E = Etot;
		Mass = sqrt(Etot*Etot - p*p);

		// Lock access to ROOT global while we access it
		pthread_mutex_lock(gROOTSPY_MUTEX);

		h_px->Fill(px);
		h_py->Fill(py);
		h_pz->Fill(pz);
		h_pvE->Fill(E,p);
		h_E->Fill(E);
		h_Mass->Fill(Mass);
		
		h_Mass_E->Fill(Mass, E);
		
		// Limit how large the TTree can get
		if(Nevents<MAX_TREE_EVENTS){
			T->Fill();
		}else if(Nevents == MAX_TREE_EVENTS){
			cout << MAX_TREE_EVENTS/1000 << "k events in tree. Only histograms will be filled from here on." << endl;
		}

		// Release lock on ROOT global
		pthread_mutex_unlock(gROOTSPY_MUTEX);
		
		if(((++Nevents) % 100)==0){
		  //gDirectory->ls();
		  
		  cout <<" "<<Nevents<<" events simulated "<<endl;
		  
		  //cout<<" "<<Nevents<<" events simulated "<<"\r";
		  //cout.flush();
		}

		// sleep a bit in order to limit how fast the histos are filled
		usleep(5000);
	}

	cout << endl;
	cout << "Ending" << endl;
	
	delete h_px;
	delete h_py;
	delete h_pz;
	delete h_pvE;
	delete h_E;
	delete h_Mass;
	delete h_Mass_E;
	delete T;
	delete drootspy;

	return 0;
}


