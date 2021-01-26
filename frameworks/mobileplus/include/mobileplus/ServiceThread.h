#ifndef MOBILEPLUS_SERVICETHREAD_H
#define MOBILEPLUS_SERVICETHREAD_H

#include <utils/threads.h>
#include <messaging/Looper.h>
#include <mobileplus/ServiceHandler.h>
#include <mobileplus/ConnectionManager.h>

using namespace android::messaging;

namespace android {

class ServiceHandler;
class RemoteBinderManager;
class DeathObserver;

class ServiceThread : public Thread
{
public:
	class DeathObserver : public IBinder::DeathRecipient {
		ServiceThread& mServiceThread;
		virtual void binderDied(const wp<IBinder>& who) {
			ALOG(LOG_INFO, LOG_TAG, "serviceThread died [%p]", &who);
			mServiceThread.serviceThreadDied();
		}
	public:
		DeathObserver(ServiceThread& serviceThread) : mServiceThread(serviceThread) { }
	};

	ServiceThread(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, sp<IBinder> binderNode, int32_t handle, int32_t type);
	~ServiceThread() {}
	virtual bool threadLoop();
	ServiceHandler* getHandler();
	void serviceThreadDied();

	sp<IBinder> mBinderNode;
	RemoteBinderManager *mRemoteBinderManager;
	ConnectionManager *mConnectionManager;
	int32_t mHandle;
	int32_t mType;
	Looper *mLooper;
	ServiceHandler *mHandler;
	Lock mLock;
	CondVar mCondVar;
	bool mIsDone;
	sp<IBinder::DeathRecipient> mDeathObserver;
};


}; 

#endif

