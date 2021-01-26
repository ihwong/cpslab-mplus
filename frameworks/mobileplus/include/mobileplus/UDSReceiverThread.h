#ifndef MOBILEPLUS_UDSRECEIVERTHREAD_H
#define MOBILEPLUS_UDSRECEIVERTHREAD_H

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

class UDSReceiverThread : public Thread
{
public:
	UDSReceiverThread(sp<BitTube> bitTube, int handle) {
		mBitTube = bitTube;
		mPort = SENSOR_PORT + handle;
	}
	~UDSReceiverThread() {}
	virtual bool threadLoop();
	sp<BitTube> mBitTube;
	short mPort;
};

};

#endif
