#define LOG_TAG "RemoteActivityManager.cpp"

#include <mobileplus/RemoteActivityManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>


namespace android {

RemoteActivityManager* RemoteActivityManager::activityManager(){
	return this;
}

void RemoteActivityManager::bindService(int32_t callingUid, String16 callerName, Parcel& data, Parcel *reply, uint32_t flags){

	//Parse a parcel
	const size_t* objects = (size_t*) data.objects();
	int lastIdx = data.objectsCount()-1;

	data.setDataPosition(objects[lastIdx]);
	sp<IBinder> connectionStub = data.readStrongBinder();
	int32_t connectionHandle = ((BpBinder*) connectionStub->remoteBinder())->handle();

	//get a serviceThread for connectionStub
	ServiceThread* serviceThread = mRemoteBinderManager->findServiceThread(connectionHandle);
	if(serviceThread == NULL){
		serviceThread = new ServiceThread(mRemoteBinderManager, mConnectionManager, connectionStub, connectionHandle, HandlerType::NORMAL_SERVICE_TYPE);
		mRemoteBinderManager->registerServiceThread(connectionHandle, serviceThread);
		serviceThread->run("ServiceThread");
	}

	ALOG(LOG_INFO, LOG_TAG, "bindService(), data= %p, reply= %p, Stub= %p, StubHandle = %d", &data, reply, &connectionStub, connectionHandle);

	//Register the caller app into the server device
	

	//send the parcel
	data.setDataPosition(0);
	mConnectionManager->sendParcel(mSock, String8(callerName).string(), NULL, mHandle, SharingType::REMOTE_BIND_ISOLATED_SERVICE_TRANSACTION, flags, data);
	
	wait();
	reply->setDataPosition(0);
	mReply = NULL;
}

};
