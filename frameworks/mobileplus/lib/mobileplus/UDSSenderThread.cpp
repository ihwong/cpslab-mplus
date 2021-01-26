#include <mobileplus/UDSSenderThread.h>
#define LOG_OHSANG "MOBILEPLUS(UDSSenderThread)"

namespace android {
	
bool UDSSenderThread::threadLoop() 
{
        ALOGI("A new UDSSenderThread(0x%p, port = %d) is created", this, mPort);
	int sock;
	ASensorEvent buffer[16];
	struct sockaddr_in recvAddr;
	struct pollfd uds;

	memset(&recvAddr, 0, sizeof(recvAddr));
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
		ALOGE("(SensorReceiver) Socket failed with err %d", errno);
		shutdown(sock, SHUT_RDWR);
		close(sock);
	}
	recvAddr.sin_family = AF_INET;
	recvAddr.sin_addr.s_addr = inet_addr(mDestIpAddr);
	recvAddr.sin_port = htons(mPort);
	uds.fd = mBitTube->getFd();
	uds.events = POLLIN;

	while(!Thread::exitPending()) {
		int res = poll(&uds, 1, TIME_OUT);
		if (uds.revents & POLLIN) {
			size_t recvNum = BitTube::recvObjects(mBitTube, buffer, 256);
			for(int i = 0; i < recvNum; i++)
				sendto(sock, &buffer[i], sizeof(ASensorEvent), 0, (struct sockaddr*)&recvAddr, sizeof(recvAddr));
		}
	}
	return false;
}

}; 
