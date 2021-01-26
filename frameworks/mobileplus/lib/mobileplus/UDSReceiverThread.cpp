#include <mobileplus/UDSReceiverThread.h>
#define LOG_OHSANG "MOBILEPLUS(UDSReceiverThread)"

namespace android {

bool UDSReceiverThread::threadLoop() 
{
	ALOGI("A new UDSReceiverThread(0x%p, port = %d) is created", this, mPort);
    struct sockaddr_in recvAddr, sendAddr;
	int sock, recvLen, sendLen;
	socklen_t addrLen = sizeof(sendAddr);
	ASensorEvent buffer;

	memset(&recvAddr, 0, sizeof(recvAddr));
	memset(&sendAddr, 0, sizeof(sendAddr));

	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0) {
        ALOGE("(ListenThread) Socket failed with err %d", errno);
        shutdown(sock, SHUT_RDWR);
        close(sock);
		return false;
	}

	recvAddr.sin_family = AF_INET;
	recvAddr.sin_addr.s_addr = INADDR_ANY;
	recvAddr.sin_port = htons(mPort);
	if (bind(sock, (struct sockaddr*)&recvAddr, sizeof(recvAddr)) < 0) {
        ALOGE("(ListenThread) Bind failed with err %d", errno);
        shutdown(sock, SHUT_RDWR);
        close(sock);
		return false;
	}

	while(!Thread::exitPending()) {
		int recvLen = recvfrom(sock, &buffer, sizeof(ASensorEvent), 0, (struct sockaddr*)&sendAddr, &addrLen);

		// adjusting x, y, z
		/*
		float temp = buffer.acceleration.x;
		buffer.acceleration.x = buffer.acceleration.y;
		buffer.acceleration.y = temp;
		*/
		BitTube::sendObjects(mBitTube, &buffer, 1);
	}
	return false;
}

}; 
