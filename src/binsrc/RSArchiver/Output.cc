/// helper functions to generate output in various formats

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

static string IMG_SUFFIX = "png";
//static TCanvas *c1 = NULL;

/////////////////////////////////////////////////////////////////////////////



void write_html_header(ofstream &f, string thedir)
{
    f << "<html>" << endl 
      << "<head>" << endl 
      << "<title>" << endl
      << thedir << endl
      << "</title>" << endl
      << "</head>" << endl
      << "<body>" << endl << endl;
    
    string header_str = "<h1>";
    vector<string> dirs;  // thedir.split("/");
    istringstream thedir_ss( thedir );
    while(!thedir_ss.eof()) {
	string dir;
	getline( thedir_ss, dir, '/' );
	dirs.push_back( dir );
    }

    //cout << "printing out for " << thedir << " with " 
    // std::to_string(dirs.size()) << " elements" << endl;

    for(int i=0; i<dirs.size(); i++) {
        if(i == 0)
            header_str += "/";
        else
            header_str += dirs[i] + "/";
    }
    
    header_str += "</h1>";
    f << header_str << endl << endl;
}
 
void write_html_footer(ofstream &f)
{
    f << endl << "</body>" << endl << "</html>" << endl;
}

void write_html_dirs(ofstream &f, vector<string> &dirs)
{
    f << "<ul>" << endl;
    f << "<li> <a href=\"../\">../</a>" << endl;

    for( vector<string>::const_iterator dir_it = dirs.begin();
	 dir_it != dirs.end(); dir_it++ ) {
        f << "<li> <a href=\"" + *dir_it + "\">" + *dir_it + "/</a>" << endl;
    }

    f << "</ul>" << endl << endl;
}

void write_html_hists(ofstream &f, vector<string> &hists)
{
    for( vector<string>::const_iterator hist_it = hists.begin();
	 hist_it != hists.end(); hist_it++ ) {

        string hist_file = *hist_it + "."  + IMG_SUFFIX;
        f << "<img src =\"" + hist_file + "\" alt=\"" + hist_file + "\">" << endl;
    }
}

//static bool do_mkdir(const char *path, mode_t mode)
static bool do_mkdir(string path, mode_t mode)
{
    struct stat     st;
    bool            status = true;
    
    if (stat(path.c_str(), &st) != 0) {
	// Directory does not exist. EEXIST for race condition 
	if (mkdir(path.c_str(), mode) != 0 && errno != EEXIST)
	    status = false;
    }
    else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        status = false;
    }
    
    return status;
}

/////////////////////////////////////////////////////////////////////////////



// this is recursive?
void GenerateHtmlOutput(TDirectory *root_dir, string basedir, string subdir)
{
    vector<string> dirs;
    vector<string> hists;

    ofstream html_file;
    html_file.open( (basedir + "/" + subdir + "/index.html").c_str());
    if(!html_file.is_open())
	return;

    TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);
    write_html_header(html_file, subdir);
    
    TIter next(root_dir->GetListOfKeys());
    TObject *keyobj;  
    while (keyobj = next()) {
	TKey *key = (TKey *)keyobj;
        //const char *name = key.GetName()
        string name = key->GetName();
	
        TObject *obj = root_dir->Get( key->GetName() );  // should we be extracted TNamed?
        //string obj_type = obj->Class_Name();
	//cout << name << " is an object of type " << obj_type << endl;

	// handle directories
	TDirectory *the_dir = dynamic_cast<TDirectory*>(obj);
	if(the_dir) {
	    cout << name << " is an object of type TDirectory" << endl;
	    dirs.push_back( name );

	    //if(!do_mkdir(basedir + "/" + subdir + "/" + name, "0755"))
	    if(!do_mkdir(basedir + "/" + subdir + "/" + name, 0755))
		continue;
	    
	    //GenerateHtmlOutput(the_dir, basedir, subdir+"/"+name);
	}


	// handle histograms - 1D
	TH1 *the_hist = dynamic_cast<TH1*>(obj);
	if(the_hist) {
	    cout << name << " is an object of type TH1" << endl;
	    hists.push_back( name );
	    
	    the_hist->Draw();
	    string img_filename = basedir + "/" + subdir + "/" + name + "."  + IMG_SUFFIX;
	    c1->Print(img_filename.c_str());
	}

	// handle histograms - 2D
	TH2 *the_hist2d = dynamic_cast<TH2*>(obj);
	if(the_hist2d) {
	    cout << name << " is an object of type TH2" << endl;
	    hists.push_back( name );
	    
	    the_hist->Draw("COLZ");
	    string img_filename = basedir + "/" + subdir + "/" + name + "."  + IMG_SUFFIX;
	    c1->Print(img_filename.c_str());
	}
    }

    write_html_dirs(html_file, dirs);
    write_html_hists(html_file, hists);
    
    write_html_footer(html_file);
			
    html_file.close();
    delete c1;
}

