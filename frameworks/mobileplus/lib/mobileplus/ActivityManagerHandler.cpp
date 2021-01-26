#define LOG_TAG "ActivityManagerHandler.cpp"

#include <mobileplus/ActivityManagerHandler.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>

namespace android {

ActivityManagerHandler* ActivityManagerHandler::activityManager(){
	return this;
}

void ActivityManagerHandler::handleMessage(Message& message){

	int32_t code = message.what;
	int32_t sock = message.arg1;
	uint32_t flags = message.arg2;
	Parcel *parcel = (Parcel*) message.obj; 
	Parcel *reply = (flags & REPLY_OFF) ? NULL : new Parcel();
	String16 *filePath = (String16*) message.obj2;

	ALOG(LOG_INFO, LOG_TAG, "handleMessage(), code=%d, parcel=%p", code, parcel);

	switch(code){
		case SharingType::REMOTE_BIND_ISOLATED_SERVICE_TRANSACTION : {
			bindService(sock, *parcel, reply, flags);
			mConnectionManager->sendReply(sock, mHandle, *reply);	 
		}

	}

	parcel->freeDataNoInit();
	//delete parcel;
	delete reply;
	delete filePath;

}

void ActivityManagerHandler::bindService(int32_t sock, Parcel& data, Parcel* reply, uint32_t flags){	
	int32_t RamHandle = mRemoteBinderManager->mRemoteAmHandle;
	RemoteProxy* rAM = mRemoteBinderManager->mRemoteProxyMap[RamHandle];
	const size_t* objects = (size_t*) data.objects();
	size_t objectCount = data.objectsCount(), startPos= 0, len= 0;
	int lastIdx = objectCount-1;
	
	//create a new binder node
	data.setDataPosition(objects[lastIdx]);
	sp<IBinder> connectionStub = data.readStrongBinder();
	int32_t connHandle = ((BpBinder*) connectionStub->remoteBinder())->handle();	
	RemoteProxy* remoteProxy = mRemoteBinderManager->findRemoteProxy(connHandle);
	if (remoteProxy == nullptr){
		ALOG(LOG_INFO, LOG_TAG, "bindService(), new RemoteProxy(connHandle= %d)", connHandle);
		remoteProxy = new RemoteProxy(mRemoteBinderManager, mConnectionManager, sock, connHandle);
		mRemoteBinderManager->registerRemoteProxy(connHandle, remoteProxy);
	}
	
	Parcel request;
	startPos=0, len=0;
	for (int i=0; i<objectCount; i++){
		len = objects[i] - startPos;
		request.appendFrom(&data, startPos, len);

		if (i==lastIdx){
			request.writeStrongBinder(remoteProxy->mBinderNode);
		}
		else{
			request.writeStrongBinder(rAM->mBinderNode);
		}

		data.setDataPosition(objects[i]);
		data.readStrongBinder();
		startPos = data.dataPosition();
	}
	request.appendFrom(&data, startPos, data.dataSize()-startPos);

	IPCThreadState::self()->transact(mHandle, ActivityManager::TRANSACTION_REMOTE_BIND_ISOLATED_SERVICE, request, reply, 0);
}



};
