

#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>


#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
using namespace std;


class rs_netdevice{
	public:
		
		rs_netdevice(struct in_addr *sin_addr, string name):sin_addr(sin_addr),name(name){
			
			addr32 = ntohl( (uint32_t)(sin_addr->s_addr) );
			addr8 = (uint8_t*)&addr32;
			ip_dotted = inet_ntoa(*sin_addr);
		}
	
		struct in_addr *sin_addr;
		string name;
		string ip_dotted;
		uint32_t addr32;
		uint8_t *addr8;
		
		//-------------------------------
		// CalcRank
		uint32_t CalcRank(uint8_t *match_addr8)
		{
		
			/// Calculate the rank of this device based on the given
			/// IP matching address. The match_addr8 array, if not NULL, 
			/// should be 4 bytes long containing the IP address to match
			/// to. (This is a poor man's way of matching the subnet.)
			/// If match_addr8 is NULL or contains all zeros, then the ranking
			/// is done without consideration of any address.
		
			uint32_t match_addr32 = 0;
			if( match_addr8 != NULL ) match_addr32 = *(uint32_t*)match_addr8;

			// We want to choose the best (fastest) device.
			uint32_t rank = 0;
			
			// infiniband is preferred
			if(name.find("ib")==0) rank += 8;
			
			// promote all devices over loopback device
			if(name.find("lo")!=0) rank += 1;
			
			// first ethernet device tends to be eth0
			// second may be eth1 or eth2, but for Hall-D
			// the second is always the 10GB card vs. the 1GB eth0
			if(name.find("eth")==0 && name!="eth0") rank +=2;
			
			// Only match subnet if it is not all zeros
			if(match_addr32!=0){
				uint8_t *addr8 = (uint8_t*)&addr32;
				for(int i=3; i>=0; i--){
					if(match_addr8[i] != addr8[i]) break;
					rank += 10;
				}
			}
			
			return rank;
		}

		//===================== static methods ==================

		//-------------------------------
		// RankDeviceList
		static void RankDeviceList(vector<rs_netdevice*> &devs, uint8_t *addr8)
		{
			/// Re-order the given list of network devices
			/// in order of preference using the given ip
			/// address. addr8 should be a 4 element array
			/// containing the ip address or subnet we want
			/// to match to. If it is NULL or all zeros, then
			/// ranking is done without consideration of the
			/// subnet.
			
			// Calculate rankings
			map<rs_netdevice*, uint32_t> ranks;
			for(uint32_t i=0; i<devs.size(); i++) ranks[devs[i]] = devs[i]->CalcRank(addr8);

			// Make new vector ordered by rankings
			vector<rs_netdevice*> ordered_devs;
			while(!ranks.empty()){
				map<rs_netdevice*, uint32_t>::iterator it;
				map<rs_netdevice*, uint32_t>::iterator itbest = ranks.begin();
				uint32_t max_rank = 0;
				for(it=ranks.begin(); it!=ranks.end(); it++){
					if(it->second > max_rank){
						max_rank = it->second;
						itbest = it;
					}
				}
				ordered_devs.push_back(itbest->first);
				ranks.erase(itbest);
			}
			
			// Replaced contents of input vector with ordered vector
			devs =  ordered_devs;
		}

		//-------------------------------
		// GetDeviceList
		static void GetDeviceList(vector<rs_netdevice*> &devs)
		{
			/// Static method for getting list of network
			/// devices.
		
			struct ifaddrs *ifap = NULL;
			getifaddrs(&ifap);
			while(ifap){

				// Only interested in AF_INET devices
				if(ifap->ifa_addr) {
					if(ifap->ifa_addr->sa_family == AF_INET){
						struct sockaddr_in *addr = (struct sockaddr_in*)ifap->ifa_addr;
						uint32_t addr32 = ntohl( (uint32_t)(addr->sin_addr.s_addr) );
						string name = ifap->ifa_name;

						if(addr32!=0){					
							rs_netdevice *dev = new rs_netdevice(&addr->sin_addr, name);
							devs.push_back(dev);
						}
					}
				}

				// Advance to next device (if any)
				ifap = ifap->ifa_next;
			}
			
			// Rank list without consideration of IP address/subnet
			RankDeviceList(devs, NULL);
		}

		//-------------------------------
		// PrintDevices
		static void PrintDevices(vector<rs_netdevice*> &devs)
		{
			cout << "Available network devices" << endl;
			cout << "-------------------------" << endl;
			for(uint32_t i=0; i<devs.size(); i++){

				rs_netdevice *dev = devs[i];

				string &name = dev->name;
				string &ip_dotted = dev->ip_dotted;
				uint32_t addr32 = dev->addr32;
				uint8_t *addr8 = (uint8_t*)&addr32;

				string pad1 = string(8-name.length(),' ');
				string pad2 = string(15-ip_dotted.length(),' ');
				cout << name << pad1 << ip_dotted << pad2 << (i==0 ? "<--":"") << endl;
			}
			cout << endl;
		}
};



