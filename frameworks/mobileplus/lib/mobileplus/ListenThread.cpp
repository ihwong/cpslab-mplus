#define LOG_TAG "ListenThread.cpp"

#include <mobileplus/ListenThread.h>

namespace android {

bool ListenThread::threadLoop()
{
	ALOG(LOG_INFO, LOG_TAG, "ListenThread tid = %d", gettid());
    if (!openListenSock()) {
		ALOG(LOG_INFO, LOG_TAG, "Failed to open listen socket");
		return false;
    }
	else {
		ALOG(LOG_INFO, LOG_TAG, "ListenSock is opened %d", mListenSock);
		mSocks[0].fd = mListenSock;
		mSocks[0].events = POLLIN;
	}
	while(!Thread::exitPending()) {
		int res = poll(mSocks, MAX_SOCK_NUM, TIME_OUT);
		if (mSocks[0].revents & POLLIN) {
			struct sockaddr_in clientAddr;
			socklen_t clientLen = sizeof(clientAddr);
			int clientSock = accept(mListenSock, (struct sockaddr*)&clientAddr, &clientLen);
			if (clientSock < 0) {
				ALOG(LOG_INFO, LOG_TAG,"Failed accept on socket %d with err %d", mListenSock, errno);
			} else {
				if (!setNodelay(clientSock))
					cleanupSock(clientSock);
				else {
					ALOG(LOG_INFO, LOG_TAG,"New client is connected with sock = %d", clientSock);
					mSocks[clientSock].fd = clientSock;
					mSocks[clientSock].events = POLLIN;
					// naive implementation
					mConnectionManager->mClientSock = clientSock;
					mConnectionManager->mClientIpAddr = inet_ntoa(clientAddr.sin_addr);
				}
			}
			if (--res <= 0)
				continue;
		}
		
		for (int i = 1; i < MAX_SOCK_NUM; i++) {
			if (mSocks[i].fd < 0)
				continue;
			if (mSocks[i].revents & POLLIN) {
				if (!mConnectionManager->receiveMessage(mSocks[i].fd)) {
					cleanupSock(mSocks[i].fd);
					mSocks[i].fd = -1;
				}
				if (--res <= 0)
					break;
			}
		}
	}
	return false;
}

bool ListenThread::openListenSock()
{
    bool ret = false;
    struct sockaddr_in addr;
    cleanupSock(mListenSock);
    while ((mListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		ALOG(LOG_INFO, LOG_TAG,"socket() is failed with err %d", errno);
        //goto bailout;
		if(!ret)
			cleanupSock(mListenSock);
    }
    if (!setNonblocking(mListenSock) || !setReuseaddr(mListenSock))
        goto bailout;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTEN_PORT);
    while (bind(mListenSock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		ALOG(LOG_INFO, LOG_TAG,"bind() is failed with err %d", errno);
        goto bailout;
    }
    while (listen(mListenSock, 5) < 0) {
		ALOG(LOG_INFO, LOG_TAG,"listen() is failed with err %d", errno);
        goto bailout;
    }
    ret = true;
bailout:
    if (!ret)
        cleanupSock(mListenSock);
	return ret;
}

void ListenThread::cleanupSock(int32_t sock) {
    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
}

bool ListenThread::setNonblocking(int32_t sock) {
    int32_t flags = fcntl(sock, F_GETFL);
    while (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		ALOG(LOG_INFO, LOG_TAG,"Failed to set socket (%d) to non-blocking mode (errno %d)", sock, errno);
        //return false;
		sleep(1);
    }
    return true;
}

bool ListenThread::setNodelay(int32_t sock) {
    int32_t tmp = 1;
    while (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &tmp, sizeof(tmp)) < 0) {
		ALOG(LOG_INFO, LOG_TAG,"Failed to set socket (%d) to no-delay mode (errno %d)", sock, errno);
       // return false;
		sleep(1);
    }
    return true;
}

bool ListenThread::setReuseaddr(int32_t sock) {
	int32_t sockopt = 1;
	while(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &sockopt, sizeof(sockopt)) == -1) {
		ALOG(LOG_INFO, LOG_TAG,"Failed to set socket (%d) to SO_REUSEADDR (errno %d)", sock, errno);
        //return false;
		sleep(1);
	}
	return true;
}

}; 
