#ifndef MOBILEPLUS_LISTENTHREAD_H
#define MOBILEPLUS_LISTENTHREAD_H

#include <linux/in.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <utils/threads.h>
#include <mobileplus/ConnectionManager.h>

namespace android {
	
class ConnectionManager;
class ListenThread : public Thread
{
	friend class ConnectionManager;
public:
	ListenThread(ConnectionManager *c) {
		mConnectionManager = c;
		mListenSock = 0;
	}
	~ListenThread() {}
	virtual bool threadLoop();
	bool openListenSock();
	void cleanupSock(int32_t sock);
	bool setNonblocking(int32_t sock);
	bool setNodelay(int32_t sock);
	bool setReuseaddr(int32_t sock);
//private:
	ConnectionManager *mConnectionManager;
	int mListenSock;
	struct pollfd mSocks[MAX_SOCK_NUM];
};

}; 
#endif
