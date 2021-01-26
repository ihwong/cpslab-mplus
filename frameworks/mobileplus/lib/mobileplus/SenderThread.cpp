#define LOG_TAG "SenderThread.cpp"

#include <mobileplus/SenderThread.h>

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <messaging/Semaphore.h>

#include <binder/IPCThreadState.h>

#define LOG_OHSANG "MOBILEPLUS(SenderThread)"

namespace android {

SenderThread::SenderThread() : mCondVar(mLock) {
	mIsDone = false;
	ALOG(LOG_INFO, LOG_TAG,"(%d) new SenderThread()", gettid());
}

bool SenderThread::threadLoop() 
{
	ALOG(LOG_INFO, LOG_TAG,"(%d) threadLoop()", gettid());
	Looper::prepare();
	mLooper = Looper::myLooper();
	mHandler = new SenderHandler();
	mCondVar.notifyAll();
	Looper::loop();
	return false;
}

SenderHandler::SenderHandler() {}

SenderHandler* SenderThread::getHandler() 
{
	AutoLock autoLock(mLock);
	if (!mIsDone && mHandler == NULL) {
		mCondVar.wait();
	}
	return mHandler;
}

void SenderHandler::handleMessage(Message &message)
{
	int32_t sock = message.what;
	int8_t *buffer = (int8_t*) message.obj;
	int32_t msgLen = message.arg1;
	int32_t flags = message.arg2;
	int32_t sendLen = send(sock, buffer, msgLen, flags);
	ALOG(LOG_INFO, LOG_TAG,"(%d) handleMessage(), send a message (sendLen = %d), sock = %d, buffer = %p, msgLen = %d, flags = %d", gettid(), sendLen, sock, &buffer, msgLen, flags);
	delete[] buffer;
}

void SenderHandler::transact(int32_t handle, uint32_t code, Parcel& request, Parcel* reply, int32_t flags)
{
	Semaphore sema;
	Message& msg = obtainMessage(0);
	msg.arg1 = handle;
	msg.arg2 = code;
	msg.arg3 = flags;
	msg.obj = &request;
	msg.obj2 = reply;
	msg.obj3 = &sema;
	sendMessage(msg);
	sema.wait();
}

}; 
