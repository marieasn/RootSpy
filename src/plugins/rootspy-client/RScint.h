
// 
// In order for the ROOT cint to use the subroutines
// in the plugin, we have to define a ROOT dictionary.
// This provides the interface for calling these. This
// file should contain the minimal "glue" to redirect
// calls to the RScint class to the subroutines doing
// the actual work in rootspy-client.cc. Bringing in
// additional classes or headers here is likely to cause
// rootcint to choke.
//

#ifndef _RSCINT_
#define _RSCINT_

#include <TObject.h>
#include <TH1.h>

extern "C" {
bool InitRootSpy(void);
void rsHelp(void);
TH1* rsGet(const char *hnamepath);
void rsList(void);
};

class RScint:public TObject
{
	public:
		
		RScint(void){ initialized = InitRootSpy(); }

		static void Help(void){ rsHelp(); }
		void List(void){ rsList(); }
		TH1* Get(const char *hnamepath){ return rsGet(hnamepath); }

	private:
		bool initialized;

	ClassDef(RScint,1)
};


#endif // _RSCINT_
