// $Id$
//
//    File: rs_info.cc
// Created: Sat Aug 29 07:42:56 EDT 2009
// Creator: davidl (on Darwin Amelia.local 9.8.0 i386)
//

#include "rs_info.h"
#include "rs_cmsg.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
using namespace std;


extern rs_info *RS_INFO;


//...................................................
// REGISTER_ROOTSPY_MACRO
//
// This is similar to the version DRootSpy.cc, but is here to provide
// a means for the RootSpy GUI program to directly attach plugins
// and use their macros. This is primarily intended for use when reading
// from a ROOT file so that one can test the macros without needing a
// cMsg server.
extern "C"{
void REGISTER_ROOTSPY_MACRO(string name, string path, string macro_data){
	if(RS_INFO) RS_INFO->LoadMacro(name, path, macro_data);
}
};
//...................................................


bool sortPredicate(const hid_t& hid1, const hid_t& hid2){
	if(hid1.serverName != hid2.serverName){
		return hid1.serverName < hid2.serverName;
	} else{
		return hid1.hnamepath < hid2.hnamepath;
	}
}

//---------------------------------
// rs_info    (Constructor)
//---------------------------------
rs_info::rs_info()
{
	pthread_mutex_init(&mutex, NULL);

	current.serverName = "N/A";
	current.hnamepath = "-------------------------------------------------------";
	
	sum_dir = new TDirectory("rootspy", "RootSpy local");
	reset_dir = new TDirectory("rootspy_snapshots", "RootSpy local snapshots");
	update = false;

	rootfile = NULL;
}

//---------------------------------
// ~rs_info    (Destructor)
//---------------------------------
rs_info::~rs_info()
{
	if(sum_dir != NULL) {delete sum_dir;}
	
	if(rootfile != NULL) {rootfile->Close(); delete rootfile;}
}

//---------------------------------
// GetActiveHIDs
//---------------------------------
void rs_info::GetActiveHIDs(vector<hid_t> &active_hids)
{
	// This should be called while RS_INFO->Lock is active to ensure
	// RS_INFO isn't changing. We don't lock it here since the methods
	// calling us also need to access RS_INFO and we need to make sure
	// it is not changing through the whole call chain.

	// Make list of all active hids
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.begin();
	for(; hdef_iter!=RS_INFO->histdefs.end(); hdef_iter++){
		map<string, bool>::iterator server_iter = hdef_iter->second.servers.begin();
		for(; server_iter!=hdef_iter->second.servers.end(); server_iter++){
			
			const string &server = server_iter->first;
			const string &hnamepath = hdef_iter->first;
			
			// Find server_info_t object to see if server itself is active
			map<string,server_info_t>::iterator server_info_iter = RS_INFO->servers.find(server);
			if(server_info_iter!=RS_INFO->servers.end()){
	
				bool server_active = server_info_iter->second.active;
				bool hdef_active = hdef_iter->second.active;
				bool hid_active = server_iter->second;
				bool active;
				
				// The histogram is "active" based on the flags and the viewStyle
				if(RS_INFO->viewStyle==rs_info::kViewByObject){
					active = server_active && hid_active;
				}else{
					active = hdef_active && hid_active;
				}
				
				if(active){
					hid_t hid(server, hnamepath);
					active_hids.push_back(hid);
				}
			}
		}
	}
	
}

//---------------------------------
// RequestHistograms
//---------------------------------
int rs_info::RequestHistograms(string hnamepath, bool lock_mutex)
{
	/// Request the specified histogram or macro from all servers
	/// that provide it. If the lock_mutex value is set to false,
	/// then the RS_INFO lock will not be locked during the operation.
	/// In that case it is assumed that the calling routine has already
	/// locked it.

	int NrequestsSent = 0;

	if(lock_mutex) Lock();

	map<string,hdef_t>::iterator it = histdefs.find(hnamepath);
	if(it != histdefs.end()){
		
		// Loop over servers, requesting this hist from each one (that is active)
		map<string, bool> &servers = it->second.servers;
		map<string, bool>::iterator server_iter = servers.begin();
		for(; server_iter!=servers.end(); server_iter++){
			//if(server_iter->second){
				NrequestsSent++;
				if(it->second.type == hdef_t::macro)
					RS_CMSG->RequestMacro(server_iter->first, hnamepath);
				else
					RS_CMSG->RequestHistogram(server_iter->first, hnamepath);
			//}
		}
	}
	
	if(lock_mutex) Unlock();
	
	return NrequestsSent;
}

//---------------------------------
// RequestTrees
//---------------------------------
int rs_info::RequestTrees(string tnamepath, int64_t num_entries, bool lock_mutex)
{
	/// Request the specified histogram or macro from all servers
	/// that provide it. If the lock_mutex value is set to false,
	/// then the RS_INFO lock will not be locked during the operation.
	/// In that case it is assumed that the calling routine has already
	/// locked it.

	int NrequestsSent = 0;

	if(lock_mutex) Lock();

	map<string,hdef_t>::iterator it = histdefs.find(tnamepath);
	if(it != histdefs.end()){

		// split hnamepath into histo name and path
		string tname = tnamepath;
		string path = "/";
		size_t pos = tnamepath.find_last_of("/");
		if(pos == string::npos){
			string tname = tnamepath.substr(pos+1, tnamepath.length()-1);
			string path = tnamepath.substr(0, pos);
			if(path[path.length()-1]==':')path += "/";
		}
		
		// Loop over servers, requesting this hist from each one (that is active)
		map<string, bool> &servers = it->second.servers;
		map<string, bool>::iterator server_iter = servers.begin();
		for(; server_iter!=servers.end(); server_iter++){
			NrequestsSent++;
			RS_CMSG->RequestTree(server_iter->first, tname, path, num_entries);
		}
	}
	
	if(lock_mutex) Unlock();
	
	return NrequestsSent;
}

//---------------------------------
// GetSumHist
//---------------------------------
TH1* rs_info::GetSumHist(string &hnamepath, hdef_t::histdimension_t *type, double *sum_hist_modified, string *servers_str)
{
	/// Look through list of currently defined histograms/macros and return info on the
	/// summed histogram. If the histogram is found, a pointer to it is returned.
	/// If the type, sum_hist_modified, and servers_str pointers are given, then the values
	/// are copied into there from the the hdef_t object. If the hnamepath is not found, then
	/// NULL is returned and the type is set to hdef_t::noneD.
	///
	/// The value copied into servers_str (if not NULL) is a representation of the servers
	/// contributing to the sum. If a single server is all that is contributing, then that
	/// server is copied in. If more than one, then a string starting with "(N svrs)"
	/// is copied followed by the names or partial names of the first few servers such that
	/// the string does not exceed 25 characters. This is primarily here for the RootSpy
	/// GUI for it to put into the "server" label when displaying a hist.

	Lock();

	TH1 *sum_hist = NULL;
	if(type) *type = hdef_t::noneD;
	if(sum_hist_modified) *sum_hist_modified = 0.0;
	
	map<string,hdef_t>::iterator it = histdefs.find(hnamepath);
	if(it != histdefs.end()){
		hdef_t &hdef = it->second;

		sum_hist = hdef.sum_hist;
		if(type) *type = hdef.type;
		if(sum_hist_modified) *sum_hist_modified = hdef.sum_hist_modified;
		if(servers_str){
			if(hdef.hists.size() == 0){
				*servers_str = "<none>";
			}else if(hdef.hists.size() == 1){
				*servers_str = (hdef.hists.begin())->first;
			}else{
				stringstream ss;
				ss << "(" << hdef.hists.size() << " servers)\n";
				map<string, hinfo_t>::iterator it_srv = hdef.hists.begin();
				int Nlines=1;
				for(; it_srv!=hdef.hists.end(); it_srv++){
					ss << it_srv->first << "\n";
					if(++Nlines >=6){
						ss << "...";
					}
				}
				*servers_str = ss.str();
			}
		}
	}

	Unlock();
	
	return sum_hist;
}

//---------------------------------
// FindNextActive
//---------------------------------
hid_t rs_info::FindNextActive(hid_t &current)
{
	// Get list of all active hids
	unsigned int current_index=0;
	vector<hid_t> active_hids;
	GetActiveHIDs(active_hids);
	if(active_hids.size()<1){return current;}
	
	// Since we have at least one HID, it is time to sort them. //
	std::sort(active_hids.begin(), active_hids.end(), sortPredicate);
	
	// Find index of "current" hid
	for(unsigned int i=0; i<active_hids.size(); i++){
		if(active_hids[i]==current){
			current_index = i;
			break;
		}
	}

	// If viewing by server, just set to the next object. If viewing by object, then
	// we need to loop until we find the next hnamepath
	if(RS_INFO->viewStyle==rs_info::kViewByObject){
		for(unsigned int i=0; i<active_hids.size(); i++){
			hid_t &hid = active_hids[(i+current_index+1)%active_hids.size()];
			if(hid.hnamepath!=current.hnamepath){
				return hid;
			}
		}
	}else{
		return active_hids[(current_index+1)%active_hids.size()];
	}

	return hid_t("none", "none");  // avoid compiler warning
}

//---------------------------------
// FindPreviousActive
//---------------------------------
hid_t rs_info::FindPreviousActive(hid_t &current)
{
	// Get list of all active hids
	unsigned int current_index=0;
	vector<hid_t> active_hids;
	GetActiveHIDs(active_hids);
	if(active_hids.size()<1){return current;}
	// Since we have at least one HID, it is time to sort them. //
	std::sort(active_hids.begin(), active_hids.end(), sortPredicate);
	
	// Find index of "current" hid
	for(unsigned int i=0; i<active_hids.size(); i++){
		if(active_hids[i]==current){
			current_index = i;
			break;
		}
	}

	// If viewing by server, just set to the previous object. If viewing by object, then
	// we need to loop until we find the previous hnamepath
	if(RS_INFO->viewStyle==rs_info::kViewByObject){
		for(unsigned int i=active_hids.size(); i>0; i--){
			hid_t &hid = active_hids[(i+current_index-1)%active_hids.size()];
			if(hid.hnamepath!=current.hnamepath){
				return hid;
			}
		}
	}else{
		return active_hids[(active_hids.size()+current_index-1)%active_hids.size()];
	}
	
	return hid_t("none", "none");  // avoid compiler warning
}

//---------------------------------
// LoadRootFile
//---------------------------------
void rs_info::LoadRootFile(string fname)
{
	rootfile = new TFile(fname.c_str());
	if(!rootfile->IsOpen()){
		cout << "Unable to open ROOT file \""<<fname<<"\" for reading!" << endl;
		exit(-1);
	}
	
	// Register all ROOT histogram and tree objects from all directory levels 
	AddRootObjectsToList(rootfile);
}

//---------------------------------
// AddRootObjectsToList
//
// (This is used when reading from a local root file)
// This method recursively traverses through histogram directories making hinfo_t 
// objects out of individual histograms and then adds the info_t objects to a list.
//---------------------------------
void rs_info::AddRootObjectsToList(TDirectory *dir)
{
	dir->ReadAll();
	string path = dir->GetPath();
	
	// Chop file part off of path and make it memory-relative for bookkeeping purposes
	size_t pos = path.find_first_of(":");
	if(pos != string::npos) path = "/" + path.substr(pos+1);

	TList *list = dir->GetList();
	TIter next(list);
	while(TObject *obj = next()) {

		// If object is a histogram, then register it
		TH1 *hHist = dynamic_cast<TH1*>(obj);
		if(hHist) {
		
			// Register histogram definition
			vector<string> hist_names;
			vector<string> hist_types;
			vector<string> hist_paths;
			vector<string> hist_titles;

			hist_names.push_back(obj->GetName());
			hist_types.push_back(obj->ClassName());
			hist_paths.push_back(path);
			hist_titles.push_back(hHist->GetTitle());

			cMsgMessage msg;
			msg.add("hist_names", hist_names);
			msg.add("hist_types", hist_types);
			msg.add("hist_paths", hist_paths);
			msg.add("hist_titles", hist_titles);
			RS_CMSG->RegisterHistList("localfile", &msg);
			
			// Register histogram itself
			msg.payloadReset();

			string hnamepath = path + '/' + obj->GetName();

			TMessage *tm = new TMessage(kMESS_OBJECT);
			tm->WriteObject(obj);
			msg.setByteArray(tm->Buffer(),tm->Length());
			msg.add("hnamepath", hnamepath);
			msg.setText("histogram");
			RS_CMSG->RegisterHistogram("localfile", &msg);

			// Finished with TMessage object. Free it and release lock on ROOT global
			delete tm;
		}

		// If object is a tree, then register it
		TTree *hTree = dynamic_cast<TTree*>(obj);
		if(hTree) {
			TObjArray *branch_list = hTree->GetListOfBranches();
			vector<string> branch_info;
			TraverseTree(branch_list, branch_info);
			if(!branch_info.empty()) {
				cMsgMessage msg;
				msg.add("tree_name", obj->GetName());
				msg.add("tree_title", obj->GetTitle());
				msg.add("tree_path", dir->GetPath());
				msg.add("branch_info", branch_info);
				msg.setText("tree info");
				RS_CMSG->RegisterTreeInfo("localfile", &msg);

				// Register tree itself
				msg.payloadReset();
				
				TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");
				TTree *tree_copy = hTree->CloneTree(0);
				if(!tree_copy){
					cout << "Unable to make temporary copy of tree for transport!" << endl;
					if(f) { f->Close(); delete f; f = NULL;}
					return;
				}

				// Copy requested number of entries
				int64_t nentries = hTree->GetEntries();
				for(int64_t i=0; i<nentries; i++) {
					hTree->GetEntry(i);
					tree_copy->Fill();
				}

				// Serialize object and put it into a response message
				TMessage *tm = new TMessage(kMESS_ANY);
				f->Write();
				tm->WriteTString(f->GetName());
				tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
				f->CopyTo(*tm);

				msg.setByteArray(tm->Buffer(),tm->Length());
				msg.add("tree_name", string(obj->GetName()));
				msg.add("tree_path", string(dir->GetPath()));
				msg.setText("tree");

				// clean up temporary copy of tree
				if( tree_copy != hTree ) {
					// see source for TTree::MergeTrees
					hTree->GetListOfClones()->Remove(tree_copy);
					tree_copy->ResetBranchAddresses();
					delete tree_copy;
				}
				if(f) {
					f->Close();
					delete f;
					f = NULL;
				}

				// Finished with TMessage object. Free it and release lock on ROOT global
				delete tm;
			}
		}

		// If this happens to be a directory, recall ourself to find objects starting from it.
		TDirectory *subdir = dynamic_cast<TDirectory*>(obj);
		if(subdir!=NULL && subdir!=dir) AddRootObjectsToList(subdir);
	}
}

//---------------------------------
// TraverseTree
//
// (This is used when reading from a local root file)
//---------------------------------
void rs_info::TraverseTree(TObjArray *branch_list, vector<string>  &treeinfo)
{

	if(!branch_list) return;
	for(int i = 0; i < branch_list->GetSize(); i++) {
		TBranch *branch = dynamic_cast<TBranch*>(branch_list->At(i));
		if(branch) {
			treeinfo.push_back(string(branch->GetName()));
		} else break;
		TraverseTree(branch->GetListOfBranches(), treeinfo);
	}
}

//---------------------------------
// LoadMacro
//---------------------------------
void rs_info::LoadMacro(string name, string path, string macro_data)
{
	// Declare macro
	vector<string> macro_names;
	vector<string> macro_paths;
	macro_names.push_back(name);
	macro_paths.push_back(path);

	cMsgMessage msg;
	msg.add("macro_names", macro_names);
	msg.add("macro_paths", macro_paths);
	msg.setText("macros list");
	RS_CMSG->RegisterMacroList("localcode", &msg);

	// for the details on TMemFile usage, see getTree()
	TMemFile *f = new TMemFile(".rootspy_tmp.root", "RECREATE");
	
	// fill the TMemFile with our payload:
	//  1) TObjString of the macro "code"
	TObjString *macro_str = new TObjString(macro_data.c_str());
	macro_str->Write("macro");

	// Serialize object and put it into a response message
	TMessage *tm = new TMessage(kMESS_ANY);
	f->Write();
	tm->WriteLong64(f->GetEND()); // see treeClient.C ROOT tutorial
	f->CopyTo(*tm);

	msg.payloadReset();
	msg.setByteArray(tm->Buffer(),tm->Length());
	msg.add("macro_name", name);
	msg.add("macro_path", path);
	msg.add("macro_version", (int32_t)1);
	msg.setText("macro");

	// clean up everything
	if(f) {
		f->Close();
		delete f;
		f = NULL;
	}

	// Finished with TMessage object. Free it and release lock on ROOT global
	delete tm;
	delete macro_str;

	// Register macro
	RS_CMSG->RegisterMacro("localfile", &msg);
}

//---------------------------------
// ResetHisto
//---------------------------------
void  rs_info::ResetHisto(const string &hnamepath)
{
	cout << "Resetting histo: " << hnamepath << endl;

	RS_INFO->Lock();

	// Get pointer to hdef_t
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
	if(hdef_iter==RS_INFO->histdefs.end()){
		RS_INFO->Unlock();
		return;
	}
	hdef_t *hdef = &(hdef_iter->second);
	if(!hdef->sum_hist){
		RS_INFO->Unlock();
		return;
	}
	
	pthread_rwlock_wrlock(ROOT_MUTEX);
	
	// If reset hist already exists, add curren sum hist to it
	if(hdef->reset_hist){
		hdef->reset_hist->Add(hdef->sum_hist);
	}else{

		// Make sure the full directory path exists
		string reset_path = "";
		TDirectory *hist_reset_dir = reset_dir;
		for(uint32_t i=0; i<hdef->dirs.size(); i++){
			reset_path += "/" + hdef->dirs[i];
			TDirectory *dir = (TDirectory*)(hist_reset_dir->Get(hdef->dirs[i].c_str()));
			if(!dir) dir = hist_reset_dir->mkdir(hdef->dirs[i].c_str());
			hist_reset_dir = dir;
		}
		
		// Clone current sum hist and store in reset_dir
		string reset_hist_name = string(hdef->sum_hist->GetName())+"__reset";
		TH1 *reset_hist = (TH1*)hdef->sum_hist->Clone(reset_hist_name.c_str());
		reset_hist->SetDirectory(hist_reset_dir);
		reset_hist->SetName(hdef->sum_hist->GetName());
		
		hdef->reset_hist = reset_hist;
	}

	// Reset the sum histo
	hdef->sum_hist->Reset();
	
	pthread_rwlock_unlock(ROOT_MUTEX);

	RS_INFO->Unlock();
}

//---------------------------------
// RestoreHisto
//---------------------------------
void  rs_info::RestoreHisto(const string &hnamepath)
{
	cout << "Restoreting histo: " << hnamepath << endl;

	RS_INFO->Lock();

	// Get pointer to hdef_t
	map<string,hdef_t>::iterator hdef_iter = RS_INFO->histdefs.find(hnamepath);
	if(hdef_iter==RS_INFO->histdefs.end()){
		RS_INFO->Unlock();
		return;
	}
	hdef_t *hdef = &(hdef_iter->second);
	
	// Can't do anything without sum and reset histograms
	if(hdef->sum_hist==NULL || hdef->reset_hist==NULL){
		RS_INFO->Unlock();
		return;
	}
	
	pthread_rwlock_wrlock(ROOT_MUTEX);
	
	// Add reset histo back into sum histo
	hdef->sum_hist->Add(hdef->reset_hist);
	
	// Delete reset hist
	delete hdef->reset_hist;
	hdef->reset_hist = NULL;

	pthread_rwlock_unlock(ROOT_MUTEX);

	RS_INFO->Unlock();
}

//---------------------------------
// Reset
//---------------------------------
void rs_info::Reset()
{
	Lock();

    // delete histograms
    for(map<hid_t,hinfo_t>::iterator hit = hinfos.begin();
	hit != hinfos.end(); hit++) {
	delete hit->second.hist;
    }

    // delete trees
    for(map<string,server_info_t>::iterator servit = servers.begin();
	servit != servers.end(); servit++) {

	for(vector<tree_info_t>::iterator treeit = servit->second.trees.begin();
	    treeit != servit->second.trees.end(); treeit++) {
	    delete treeit->tree;
	}

    }

    // delete saved histograms
    // not entirely sure from the documentation if this is the correct 
    // function - might need Delete()?
    sum_dir->Clear();  
    
    // clear up saved info
    servers.clear();
    histdefs.clear();
    hinfos.clear();

    // zero out the current histogram def
    current = hid_t();
    //current.hnamepath = "";
	
	Unlock();
}
