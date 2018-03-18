// $Id$
//
//    File: rs_serialized.h
// Created: Fri Mar 16
// Creator: davidl 
//


#ifndef _rs_serialized_
#define _rs_serialized_

// This class is used to hold a serialized TMessage
// received from a remote host. The main purpose
// is to allow deferring the extraction the TMessage
// until the main ROOT thread can do it. The message
// that came over the network can then be released.
// This is really a requirement for xMsg where only
// a refrence to the message is provided to the callback.
// This is used for both xMsg and cMsg messages.

#include <string>
#include <vector>
#include <cstdint>

class rs_serialized{
	
	public:
		
		std::string sender;
		std::string hnamepath;
		std::vector<char> data;
};


#endif // _rs_serialized_
