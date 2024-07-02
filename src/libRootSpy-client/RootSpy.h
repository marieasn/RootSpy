

#ifndef _ROOTSPY_H_
#define _ROOTSPY_H_

//#include <iostream>
//#include <iomanip>
//using namespace std;

#include </home/angen/work/2024.06.16.RootSpy/RootSpy/src/libRootSpy-client/rs_cmsg.h>
#include </home/angen/work/2024.06.16.RootSpy/RootSpy/src/libRootSpy-client/rs_xmsg.h>

class rs_mainframe;
class rs_cmsg;
class rs_info;

extern rs_mainframe *RSMF;
extern rs_cmsg *RS_CMSG;
extern rs_xmsg *RS_XMSG;
extern rs_info *RS_INFO;

#include <time.h>
#include <pthread.h>
extern pthread_rwlock_t *ROOT_MUTEX;


#ifndef _DBG_
#define _DBG_  cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl
#endif



#endif //_ROOTSPY_H_
