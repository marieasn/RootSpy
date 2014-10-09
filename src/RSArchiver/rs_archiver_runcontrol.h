#ifndef _RS_ARCHIVER_RUNCONTROL_H_
#define _RS_ARCHIVER_RUNCONTROL_H_

////#ifdef NO_CODAOBJECTROOT

//#pragma message ("Using cMsg interface...")

#include <cMsg.hxx>
using namespace cmsg;


class rs_archiver : public cMsgCallback {
    
 public:
    // constructor
 rs_archiver(const string& udl, const string& name, const string& descr, const string &theSession) : runNumber(1) {
	
	cout << "rs_archiver constructor called" << endl;

	cMsgSys = new cMsg(udl, name, descr);   
	try {                         	          
	    cMsgSys->connect(); 
	} catch (cMsgException e) {
	    cout<<endl<<endl<<endl<<endl<<"_______________  Archiver controls unable to connect to cMsg system! __________________"<<endl;
	    cout << e.toString() << endl; 
	    cout<<endl<<endl<<endl<<endl;
	    return;
	}

	// subscrube to run control requests
	cMsgSys->subscribe("runcontrol", "*", this, NULL);
	cMsgSys->start();
    }
    
    // destructor
    ~rs_archiver(void)  {
	DONE=true;
	// Stop cMsg system
	cMsgSys->stop();
    }

    //---------------------------------
    // startProcessing
    //---------------------------------
    void startProcessing()  { /* NOOP */ }

    //---------------------------------
    // getRunNumber
    //---------------------------------
    int getRunNumber()  { return runNumber; }

    //---------------------------------
    // setRunNumber
    //---------------------------------
    void setRunNumber(int newRunNumber)  { runNumber = newRunNumber; }


    //---------------------------------
    // callback
    //---------------------------------
    void callback(cMsgMessage *msg, void *userObject)
    {
	if(!msg)return;

	// The actual command is always sent in the text of the message
	if (msg->getText() == "null"){delete msg; return;}
	string cmd = msg->getText();
	
	//===========================================================
	if(cmd=="beginrun"){
	    // start archiving
	    RUN_IN_PROGRESS=true;
	}
	//===========================================================
	if(cmd=="endrun"){
	    // finish up and end the run
	    FINALIZE=true;
	    RUN_IN_PROGRESS=false;
	}
	//===========================================================
	if(cmd=="runnumber"){
	    // set run number
	    try {
		runNumber = msg->getUserInt();
	    }  catch (cMsgException e) {
		_DBG_<<"Poorly formed run number payoad. Ignoring."<<endl;
		return;
	    }
	}
	//===========================================================
	if(cmd=="exit"){
	    // time to quit
	    RUN_IN_PROGRESS=false;
	    DONE=true;
	}

	delete msg;
    }
    
 private:
    cMsg *cMsgSys;
    int runNumber;
};

#if 0

///#else

///#if __GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 8 ) ) 
///#error "CodaObject interface requires >= gcc 4.8"
///#endif

///#pragma message ("Using CODAObject interface...")


// class for communication with CODA
#include <RunObject.hxx>
using namespace codaObject;

vclass rs_archiver : public RunObject {

public:
  rs_archiver(const string& UDL, const string& name, const string& descr, const string &theSession) : 
    RunObject(UDL,name,descr) {

    cout << "rs_archiver constructor called" << endl;

    // set session if specified
    if(!theSession.empty())handleSetSession(theSession);
  }


//-----------------------------------------------------------------------------

  ~rs_archiver(void) throw()  {
    DONE=true;
  }
  
  
//-----------------------------------------------------------------------------

/*
  bool userConfigure(const string& s) throw(CodaException)  {
    paused=false;
    return(true);
  }
*/

//-----------------------------------------------------------------------------

/*
  bool userDownload(const string& s) throw(CodaException)  {
    paused=false;
    return(true);
  }
*/

//-----------------------------------------------------------------------------

/*
  bool userPrestart(const string& s) throw(CodaException)  {
    paused=false;
    return(true);
  }
*/

//-----------------------------------------------------------------------------


  bool userGo(const string& s) throw(CodaException)  {
      //PAUSED=false;
    RUN_IN_PROGRESS=true;
    return(true);
  }


//-----------------------------------------------------------------------------

/*
  bool userPause(const string& s) throw(CodaException)  {
    paused=true;
    return(true);
  }
*/

//-----------------------------------------------------------------------------

/*
  bool userResume(const string& s) throw(CodaException)  {
    paused=false;
    return(true);
  }
*/

//-----------------------------------------------------------------------------


  bool userEnd(const string& s) throw(CodaException)  {
      //paused=false;
    cout << "rs_archiver received end run command" << endl;
      FINALIZE=true;
      RUN_IN_PROGRESS=false;
      return(true);
  }


//-----------------------------------------------------------------------------

/*
  bool userReset(const string& s) throw(CodaException)  {
    paused=false;
    run_in_progress=false;
    return(true);
  }
*/

//-----------------------------------------------------------------------------


  void exit(const string& s) throw(CodaException)  {
    cout << "rs_archiver received exit command" << endl;
    RUN_IN_PROGRESS=false;
    DONE=true;
  }
  

//-----------------------------------------------------------------------------


  //void userMsgHandler(cMsgMessage *msgp, void *userArg) throw(CodaException)  {
  void userMsgHandler(cMsgMessage *msg, void *userArg) throw(CodaException)  {
    //unique_ptr<cMsgMessage> msg(msgp);
    cerr << "?et2evio...received unknown message subject,type: "
         << msg->getSubject() << "," << msg->getType() << endl << endl;
  }

};

#endif // NO_CODAOBJECTROOT

//-----------------------------------------------------------------------------




#endif
