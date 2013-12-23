/// helper class to make summary PDF

#include "PDFOutputGenerator.h"

/////////////////////////////////////////////////////////////////////////////

void PDFOutputGenerator::collect_hists(TDirectory *the_dir, vector<TH1 *> &hists)
{
  vector<TDirectory *> dirs;

  // collect information on histograms and directories
  TIter next(the_dir->GetListOfKeys());
  TObject *keyobj;  
  while (keyobj = next()) {
    TKey *key = (TKey *)keyobj;
    string name = key->GetName();
    
    TObject *obj = the_dir->Get( key->GetName() );  // should we be extracted TNamed?
    
    // handle directories
    TDirectory *the_dir = dynamic_cast<TDirectory*>(obj);
    if(the_dir) {
      //cout << name << " is an object of type TDirectory" << endl;
      //collect_hists(the_dir, hists);
      dirs.push_back( the_dir );
    }
    
    // handle histograms - 1D
    TH1 *the_hist = dynamic_cast<TH1*>(obj);
    if(the_hist) {
      hists.push_back( the_hist );
    }
    
    // handle histograms - 2D
    TH2 *the_hist2d = dynamic_cast<TH2*>(obj);
    if(the_hist2d) {
      hists.push_back( (TH1*)the_hist2d );
    }
  }
  
  // recurse in directories at the end of the function to preserve ordering
  // so that all the histograms in one directory print out together
  for(vector<TDirectory *>::iterator dir_it = dirs.begin(); 
      dir_it != dirs.end(); dir_it++ ) {
    collect_hists(*dir_it, hists);
  }

}



/////////////////////////////////////////////////////////////////////////////



void PDFOutputGenerator::GenerateOutput(TDirectory *root_dir, string basedir)
{
    vector<TH1 *> hists;
    int nhist = 0;

    string pdf_filename = basedir + "/summed_hists.pdf";

    collect_hists(root_dir, hists);

    for(vector<TH1 *>::iterator hit = hists.begin();
	hit != hists.end(); hit++, nhist++) {
      (*hit)->Draw();
      
      // write to file with proper formatting
      if(nhist == 0)
	output_canvas->Print( (pdf_filename+"(").c_str() );
      else if(nhist == hists.size()-1)
	output_canvas->Print( (pdf_filename+")").c_str() );
      else
	output_canvas->Print( pdf_filename.c_str() );
      
    }


}

