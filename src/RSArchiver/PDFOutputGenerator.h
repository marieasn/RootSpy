// helper class to make summary PDF

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream> 

using namespace std;

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <TKey.h>
// #include <TIter.h>



class PDFOutputGenerator  {

 public:
 PDFOutputGenerator()  {
    output_canvas = new TCanvas("c_pdf", "", 800, 600);  // needs to exist so that we can save image to disk
}

  void GenerateOutput(TDirectory *root_dir, string basedir);
  
 private:
  void collect_hists(TDirectory *the_dir, vector<TH1 *> &hists);

  TCanvas *output_canvas;
 

};
