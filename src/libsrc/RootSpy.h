

#ifndef _ROOTSPY_H_
#define _ROOTSPY_H_

//#include <iostream>
//#include <iomanip>
//using namespace std;

class rs_mainframe;
class rs_cmsg;
class rs_info;

extern rs_mainframe *RSMF __attribute__((weak_import));
extern rs_cmsg *RS_CMSG __attribute__((weak_import));
extern rs_info *RS_INFO __attribute__((weak_import));

#include <pthread.h>
extern pthread_rwlock_t *ROOT_MUTEX;


#ifndef _DBG_
#define _DBG_  cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl
#endif



#endif //_ROOTSPY_H_
