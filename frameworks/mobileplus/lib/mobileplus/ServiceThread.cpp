#include <mobileplus/ServiceThread.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ServiceManagerHandler.h>
#include <mobileplus/ActivityManagerHandler.h>
#include <mobileplus/ActivityTaskManagerHandler.h>
#include <mobileplus/PackageManagerHandler.h>

namespace android {

ServiceThread::ServiceThread(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, sp<IBinder> binderNode, int32_t handle, int32_t type) : mCondVar(mLock) {
	mRemoteBinderManager = remoteBinderManager;
	mConnectionManager = connectionManager;
	mBinderNode = binderNode;
	mHandle = handle;
	mType = type;
	mIsDone = false;
	mDeathObserver = new DeathObserver(*const_cast<ServiceThread *>(this));
	if (mBinderNode != NULL)
		mBinderNode->linkToDeath(mDeathObserver);
		ALOG(LOG_INFO, LOG_TAG, "A new ServiceThread (mHandle = %d) is created", mHandle);
}

bool ServiceThread::threadLoop() {
	ALOG(LOG_INFO, LOG_TAG,"ServiceThread tid = %d, mHandle = %d", gettid(), mHandle);
	Looper::prepare();
	mLooper = Looper::myLooper();
	if (mType == HandlerType::SERVICE_MANAGER_TYPE) 
		mHandler = new ServiceManagerHandler(mRemoteBinderManager, mConnectionManager, mHandle);
	else if (mType == HandlerType::ACTIVITY_MANAGER_TYPE) 
		mHandler = new ActivityManagerHandler(mRemoteBinderManager, mConnectionManager, mHandle);
	else if (mType == HandlerType::ACTIVITY_TASK_MANAGER_TYPE)
		mHandler = new ActivityTaskManagerHandler(mRemoteBinderManager, mConnectionManager, mHandle);
	else if (mType == HandlerType::PACKAGE_MANAGER_TYPE) 
		mHandler = new PackageManagerHandler(mRemoteBinderManager, mConnectionManager, mHandle);
	else if (mType == HandlerType::NORMAL_SERVICE_TYPE) 
		mHandler = new ServiceHandler(mRemoteBinderManager, mConnectionManager, mHandle);
	mCondVar.notifyAll();
	Looper::loop();
	return false;
}

ServiceHandler* ServiceThread::getHandler() {
	AutoLock autoLock(mLock);
	if (!mIsDone && mHandler == NULL) {
		mCondVar.wait();
	}
	return mHandler;
}

void ServiceThread::serviceThreadDied() {
	ALOG(LOG_INFO, LOG_TAG, "KILLED, mHandle = %d", mHandle);
	mRemoteBinderManager->eraseServiceThread(mHandle);
	delete this;
}
}; 

