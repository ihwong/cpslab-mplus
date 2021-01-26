#ifndef MOBILEPLUS_SENDERTHREAD_H
#define MOBILEPLUS_SENDERTHREAD_H

#include <sys/socket.h>
#include <utils/threads.h>
#include <messaging/Handler.h>
#include <messaging/Message.h>
#include <messaging/Looper.h>
#include <mobileplus/Global.h>
#include <binder/Parcel.h>

using namespace android::messaging;

namespace android {

class SenderHandler;
class SenderThread : public Thread
{
public:
	SenderThread();
	~SenderThread() {}
	virtual bool threadLoop();
	SenderHandler* getHandler();
private:
	Looper *mLooper;
	SenderHandler *mHandler;
	Lock mLock;
	CondVar mCondVar;
	bool mIsDone;
};

class SenderHandler : public Handler
{
public:
	SenderHandler();
	~SenderHandler() {}
	void transact(int32_t handle, uint32_t code, Parcel& request, Parcel* reply, int32_t flags);
	virtual void handleMessage(Message& message);
private:
};

}; 

#endif
