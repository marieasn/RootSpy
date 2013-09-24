

#ifndef _RSARCHIVER_H_
#define _RSARCHIVER_H_

#include <iostream>
#include <iomanip>
#include <memory>
using namespace std;


class rs_mainframe;
class rs_cmsg;
class rs_info;

extern rs_mainframe *RSMF;
extern rs_cmsg *RS_CMSG;
extern rs_info *RS_INFO;
extern pthread_rwlock_t *ROOT_MUTEX;


#ifndef _DBG_
#define _DBG_  cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ cerr<<__FILE__<<":"<<__LINE__<<endl
#endif





#endif //_RSARCHIVER_H_
