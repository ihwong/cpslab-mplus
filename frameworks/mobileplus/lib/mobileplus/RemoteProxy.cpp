#define LOG_TAG "RemoteProxy.cpp"

#include <mobileplus/RemoteProxy.h>
#include <mobileplus/ServiceThread.h>
#include <mobileplus/RemoteBinderManager.h>

namespace android {

RemoteProxy::RemoteProxy(RemoteBinderManager* remoteBinderManager, ConnectionManager *connectionManager, int32_t sock, int32_t handle)
{
	mRemoteBinderManager = remoteBinderManager;
	mConnectionManager = connectionManager;
	mSock = sock;
	mHandle = handle;
	mBinderNode = onAsBinder();

	ALOG(LOG_INFO, LOG_TAG, "A new RemoteProxy (mHandle = %d) is created", mHandle);
}

void RemoteProxy::handleBinderNode(int32_t type, Parcel& parcel, Parcel* remoteParcel ){

	if(type == ParcelType::LOCAL){
		sp<IBinder> binderNode = parcel.readStrongBinder();
		BpBinder *proxy = (binderNode) ? binderNode->remoteBinder() : nullptr;
		int32_t binderHandle = (proxy) ? proxy->handle() : -1;
		ServiceThread* serviceThread = mRemoteBinderManager->findServiceThread(binderHandle);

		if(!serviceThread){
			ALOG(LOG_INFO, LOG_TAG, "onTransact(), new ServiceThread(binderHandle) = %d", binderHandle);
			
			serviceThread = new ServiceThread(mRemoteBinderManager, mConnectionManager, binderNode, binderHandle, HandlerType::NORMAL_SERVICE_TYPE);
			mRemoteBinderManager->registerServiceThread(binderHandle, serviceThread);
			serviceThread->run("ServiceThread");
		}
	}
	else if (type == ParcelType::REMOTE){
	  	  /* make a coply of parcel to avoid "Attempt to read from protected data in Parcel..." */
	  /* also, take a look at RemoteServiceManager.cpp which had a similar problem */
	  Parcel test;
	  test.setData(remoteParcel->data(), remoteParcel->dataSize());
	  test.setDataPosition(0);
		int32_t binderHandle = RemoteBinderManager::getBinderHandle(test);
		/* okay, now everything is fine */
		// int32_t binderHandle = RemoteBinderManager::getBinderHandle(*remoteParcel);
		RemoteProxy* remoteProxy = mRemoteBinderManager->findRemoteProxy(binderHandle);

		if(!remoteProxy){
			ALOG(LOG_INFO, LOG_TAG, "onTransact(), new RemoteProxy(binderHandle = %d)", binderHandle);

			remoteProxy = new RemoteProxy(mRemoteBinderManager, mConnectionManager, mSock, binderHandle);
			mRemoteBinderManager->registerRemoteProxy(binderHandle, remoteProxy);
		}
		parcel.writeStrongBinder(remoteProxy->mBinderNode);
	}	

}

status_t RemoteProxy::onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
	//AutoMutex autoMutex(mTransLock);
	ALOG(LOG_INFO, LOG_TAG, "onTransact(), mHandle=%d, code= %d, data= %p, reply= %p, flags= %d, tid =%d", mHandle, code, &data, reply, flags, gettid());

	Parcel request;
	request.appendFrom(&data, 0, data.dataSize());
	request.setDataPosition(0);
	//setReply(reply);
	mReply = reply;

	const size_t* objects = (size_t*) request.objects();
	size_t objectCount = request.objectsCount();
	int type;
	const flat_binder_object* flat;

	for(int i=0; i<objectCount; i++){

		request.setDataPosition(objects[i]);
		flat = request.readObject(false);
		type = flat->hdr.type;

		if(type == BINDER_TYPE_HANDLE){
			request.setDataPosition(objects[i]);
			handleBinderNode(ParcelType::LOCAL, request);
		} 
		else {
			ALOG(LOG_INFO, LOG_TAG, "FAIL: Another objects!"); 
		}		
	}

	
	if (flags != 0){
		flags= 0;
	}

	mConnectionManager->sendParcel(mSock, NULL, NULL, mHandle, code, flags, request);

	if (!(flags & REPLY_OFF)){
		wait();
		
		Parcel temp;
		bool needToRestore = true;
		temp.appendFrom(mReply, 0, mReply->dataSize());
	 	temp.setDataPosition(0);

		objects = (size_t*) temp.objects();
		objectCount = temp.objectsCount();

		binder_size_t* refData =nullptr;
		mReply->setObjects(refData, 0);
	
		for(int i=0; i<objectCount; i++){

			temp.setDataPosition(objects[i]);
			flat = temp.readObject(false);
			type = flat->hdr.type;

			if(type == BINDER_TYPE_HANDLE){
				temp.setDataPosition(objects[i]);
				mReply->setDataPosition(objects[i]);
				handleBinderNode(ParcelType::REMOTE, *mReply, &temp);
			}
			else if (type == BINDER_TYPE_FD) {
			  uint32_t fdType = temp.readInt32();
			  
			  if (fdType == FdType::UDS) {
			    sp<BitTube> bitTube = new BitTube();
			    mUDSReceiverThread = new UDSReceiverThread(bitTube, mHandle);
			    mUDSReceiverThread->run("UDSReceiverThread");
			    mReply->setDataPosition(objects[i]);
			    // FIX IT: There's a side effect for calling writeToParcel() for the artificial parcel
			    bitTube->writeToParcel(mReply);
			  }
			} // end of else if (type == BINDER_TYPE_FD) {
			else {
				ALOG(LOG_INFO, LOG_TAG, "onTransact() fail! Another objects!");
			}
		}

		mReply->setDataPosition(0);
		if (objectCount && needToRestore){
			mReply->setObjects(objects, objectCount);
		}
	}

	return NO_ERROR;
}





RemoteServiceManager* RemoteProxy::serviceManager(){
	return NULL;
}

RemoteActivityManager* RemoteProxy::activityManager(){
	return NULL;
}

RemoteActivityTaskManager* RemoteProxy::activityTaskManager(){
	return NULL;
}

RemotePackageManager* RemoteProxy::packageManager(){
	return NULL;
}

void RemoteProxy::wait(){
	mSema.wait();
}

void RemoteProxy::signal(){
	mSema.signal();
}

void RemoteProxy::setReply(Parcel *reply){
	mReply = reply;
}

Parcel* RemoteProxy::getReply(){
	return mReply;
}

bool RemoteProxy::existReply(){
	return (mReply != nullptr) ? true : false ;
}

};
