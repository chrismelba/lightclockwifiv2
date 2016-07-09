/****************************************************************************
 *  NTP.h - gets current time from an NTP server or pool of servers.
 *		This code is a modified version of the example "Time_NTP.pde" which 
 *		is included with the time library.
 *		modified by William Moeur and made into a library: 28 MAY 2015.
 *		This code is in the public domain.
 *****************************************************************************/

#ifndef NTP_H
#define	NTP_H

#include "IPAddress.h"
#include "WiFiUdp.h"
#include <stdint.h>
#include <Time.h>


class NTP
{
  public:
    NTP();
	void begin(const char* ntpServerName, float TimeZoneOffset);
    time_t getNtpTime(void);
    void updateTimeZone(float TimeZoneOffset);


  private:
	#define LOCALPORT 2390 // local port to listen on for UDP packets
	#define NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message
	#define seventyYears 2208988800UL // Unix time starts on Jan 1 1970. that's 2208988800 seconds
	//#define SECS_PER_HOUR 3600

    void sendNTPpacket(IPAddress &address);
    uint8_t DSToffset(time_t date);
	
    WiFiUDP UDP; // A UDP instance to let us send and receive packets over UDP
	uint8_t packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
	const char* _serverName;
	time_t _syncInterval = SECS_PER_HOUR;
	float _timeZoneOffset;
};

#endif	/* NTP_H */

