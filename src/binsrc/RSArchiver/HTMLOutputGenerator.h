// helper class to make output HTML pages

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



class HTMLOutputGenerator  {

 public:
 HTMLOutputGenerator(string in_img_suffix = "png") : IMG_SUFFIX(in_img_suffix) {
    output_canvas = new TCanvas("c_html", "", 800, 600);  // needs to exist so that we can save image to disk
}

  void GenerateOutput(TDirectory *root_dir, string basedir, string subdir);
  
 private:
  void write_html_header(ofstream &f, string thedir);
  void write_html_footer(ofstream &f);
  void write_html_dirs(ofstream &f, vector<string> &dirs);
  void write_html_hists(ofstream &f, vector<string> &hists);
  bool do_mkdir(string path, mode_t mode);


  // the suffix of the generated image files determines their type
  string IMG_SUFFIX;   
  TCanvas *output_canvas;
 

};
