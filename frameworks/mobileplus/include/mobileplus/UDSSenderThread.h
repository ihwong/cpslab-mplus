#ifndef MOBILEPLUS_UDSSENDERTHREAD_H
#define MOBILEPLUS_UDSSENDERTHREAD_H

#include <mobileplus/Global.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <utils/threads.h>
#include <sensor/BitTube.h>
#include <sensor/Sensor.h>


namespace android {

class UDSSenderThread : public Thread
{
public:
	UDSSenderThread(sp<BitTube> bitTube, const char* destIpAddr, int handle) {
		mBitTube = bitTube;
		mDestIpAddr = destIpAddr;
		mPort = SENSOR_PORT + handle;
	}
	~UDSSenderThread() {}
	virtual bool threadLoop();
	sp<BitTube> mBitTube;
	const char* mDestIpAddr;
	short mPort;
};

};

#endif
