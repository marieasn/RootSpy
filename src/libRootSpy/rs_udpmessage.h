

#ifndef _rs_udpmessage_
#define _rs_udpmessage_

#include <stdint.h>

enum UDPmessageType{

	kUDP_HISTOGRAM,
	kUDP_MACRO

};

class rs_udpmessage{
	public:
	
		// this class represents a UDP message that
		// can carry a histogram or macro. It is meant
		// to overlay on a buffer that holds the
		// header info defined in this class followed
		// by the data payload. The last member
		// "buff_start" should overlay with the first
		// word of the payload buffer. The value buff_len
		// actually holds the length of the payload in
		// bytes and one should get a pointer to the 
		// payload like this:
		//
		//   rs_udpmessage *rsudp = ...;
		//   uint8_t *buff = (uint8_t*)&rsudp->buff_start;
		//

		uint32_t len; // total length of this message in bytes (including this word)
		uint32_t mess_type;
		uint8_t hnamepath[256];
		uint8_t sender[256];
		uint32_t buff_len;
		uint32_t buff_start;
};


#endif // _rs_udpmessage_

