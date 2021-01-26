#define LOG_TAG "ServiceHandler.cpp"

#include <mobileplus/ServiceHandler.h>
#include <mobileplus/ConnectionManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/RemoteProxy.h>
#include <sensor/BitTube.h>
#include <sys/types.h>
#include <sys/stat.h> // for 'struct stat'
#include <unistd.h>
#include <binder/TextOutput.h>

namespace android {

ServiceHandler::ServiceHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t handle) {
	mRemoteBinderManager = remoteBinderManager;
	mConnectionManager = connectionManager;
	mHandle = handle;

	ALOG(LOG_INFO, LOG_TAG, "A new ServiceHandle(0x%p, mHandle = %d) is created", this, mHandle);
}

void ServiceHandler::handleMessage(Message& message){
	int32_t code = message.what;
	int32_t sock = message.arg1;
	int32_t flags = message.arg2;

	Parcel *parcel = (Parcel*) message.obj;
	Parcel *reply = (flags & REPLY_OFF) ? nullptr : new Parcel();

	ALOG(LOG_INFO, LOG_TAG, "handleMessage(), mHandle= %d, code= %d, parcel= %p, reply = %p, flags= %d", mHandle, code, parcel, reply, flags);
	
	serviceCall(sock, code, *parcel, reply, flags);
	
	
	   
	if(!(flags & REPLY_OFF)){
		ALOG(LOG_INFO, LOG_TAG, "sendReply from Handle %d, code %d, reply %p", mHandle, code, reply);
		mConnectionManager->sendReply(sock, mHandle, *reply);
	}

	//parcel->freeData();
	//delete parcel;
	//reply->freeData();
	//delete reply;
}

void ServiceHandler::serviceCall(int32_t sock, int32_t code, Parcel& data, Parcel* reply, uint32_t flags){
	Parcel request, temp;
	const size_t* objects = (size_t *)data.objects();
	size_t objectsCount = data.objectsCount(), startPos = 0, len = 0;
	bool existsPreparedReply = false;
	const flat_binder_object* flat;
	int type;

	String16 InterfaceName = data.getInterface();

	for(int i=0; i<objectsCount; i++){
		len = objects[i] - startPos;
		request.appendFrom(&data, startPos, len);
		
		data.setDataPosition(objects[i]);
		flat = data.readObject(false);
		type = flat->hdr.type;
		if (type == BINDER_TYPE_HANDLE){
			data.setDataPosition(objects[i]);
			handleBinderNode(ParcelType::REMOTE, request, &data, sock);
			startPos = request.dataPosition();
		}
		else { ALOG(LOG_INFO, LOG_TAG, "FAIL: Another object!"); }
	
	}

	len = data.dataSize() - startPos;
	request.appendFrom(&data, startPos, len);

	/*
	handle "android.gui.IGraphicBufferProducer"
	*/

	ALOG(LOG_INFO, LOG_TAG, "servicecall(), interface %s", String8(InterfaceName).string());

	if(!existsPreparedReply){
		status_t status = IPCThreadState::self()->transact(mHandle, code, request, &temp, flags);
		ALOG(LOG_INFO, LOG_TAG, "serviceCall(), mHandle %d, code %d, status %d, tid %d", mHandle, code, status, gettid());
	}

	if (! (flags & REPLY_OFF)){
		ALOG(LOG_INFO, LOG_TAG, "serviceCall() flag is not REPLY_OFF" );

		reply->appendFrom(&temp, 0, temp.dataSize());
		objects = (size_t*) reply->objects();
		objectsCount = reply->objectsCount();

		for(int i=0; i<objectsCount; i++){
			reply->setDataPosition(objects[i]);
			flat = reply->readObject(false);
			type = flat->hdr.type;

			if(type == BINDER_TYPE_HANDLE){
				reply->setDataPosition(objects[i]);
				handleBinderNode(ParcelType::LOCAL, *reply);
			}
		        else if (type == BINDER_TYPE_FD) {

				uint32_t flags = reply->readInt32();
				uint32_t fd = reply->readInt32();
				int32_t fdType = getFdType(fd);
				
				if (fdType == FdType::UDS) {
					reply->setDataPosition(objects[i]);
					sp<BitTube> bitTube = new BitTube(*reply);
					mUDSSenderThread = new UDSSenderThread(bitTube, mConnectionManager->mClientIpAddr, mHandle);
					mUDSSenderThread->run("UDSSenderThread");
				}

				reply->setDataPosition(objects[i]);
				reply->readInt32();
				reply->writeInt32(fdType);
			}
			else{
				ALOG(LOG_INFO, LOG_TAG, "serviceCall(), not BINDER_TYPE_HANDLE");
			}
		}

	}


}

int ServiceHandler::getFdType(int fd)
{
	char path[100];
	int pid = getpid();
	struct stat buf;
	int fdType = -1;

	sprintf(path, "/proc/%d/fd/%d", pid, fd);
	lstat(path, &buf);
	char* linkname = new char[buf.st_size + 1];
	readlink(path, linkname, buf.st_size + 1);
	linkname[buf.st_size] ='\0';
	ALOGI("path: %s, linkname: %s", path, linkname);

	if (!strncmp(linkname, "socket", 6)) {
		ALOGI("Unix Domain Socket");
		fdType = FdType::UDS;
	}
	else if (!strcmp(linkname, "anon_inode:dmabuf")) {
		ALOGI("ION Memory");
		fdType = FdType::ION_MEMORY;
	}
	else if (!strcmp(linkname, "anon_inode:sync_fence")) {
		ALOGI("Sync Fence");
		fdType = FdType::SYNC_FENCE;
	}
	else if (!strcmp(linkname, "/dev/ashmem")) {
		ALOGI("ASH Memory");
		fdType = FdType::ASH_MEMORY;
	}

	delete[] linkname;
	return fdType;
}

void ServiceHandler::handleBinderNode(int32_t type, Parcel& parcel, Parcel* remoteParcel, int32_t sock){
	
	BpBinder* proxy;

	ALOG(LOG_INFO, LOG_TAG, "handleBinderNode type %d, parcel %p", type, &parcel);

	if(type == ParcelType::LOCAL){
		sp<IBinder> binderNode = parcel.readStrongBinder();
		proxy = (binderNode) ? binderNode->remoteBinder() : nullptr;
		int32_t binderHandle = (proxy) ? proxy->handle() : -1;
		ServiceThread* serviceThread = mRemoteBinderManager->findServiceThread(binderHandle);

		if(serviceThread == nullptr){
			ALOG(LOG_INFO, LOG_TAG, "serviceCall(), new ServiceThread(binderHandle = %d)", binderHandle);

			serviceThread = new ServiceThread(mRemoteBinderManager, mConnectionManager, binderNode, binderHandle, HandlerType::NORMAL_SERVICE_TYPE);
			mRemoteBinderManager->registerServiceThread(binderHandle, serviceThread);
			serviceThread->run("ServiceThread");
		}
	}
	else if (type == ParcelType::REMOTE){
		
		sp<IBinder> connStub = remoteParcel->readStrongBinder();
		int32_t binderHandle = ((BpBinder*) connStub->remoteBinder())->handle();
		RemoteProxy* remoteProxy = mRemoteBinderManager->findRemoteProxy(binderHandle);

		if(remoteProxy == nullptr){
			ALOG(LOG_INFO, LOG_TAG, "serviceCall(), new RemoteProxy(binderHandle= %d))", binderHandle);

			remoteProxy = new RemoteProxy(mRemoteBinderManager, mConnectionManager, sock, binderHandle);
			mRemoteBinderManager->registerRemoteProxy(binderHandle, remoteProxy);
		}
		parcel.writeStrongBinder(remoteProxy->mBinderNode);
	}
}

ServiceManagerHandler* ServiceHandler::serviceManager()
{
	return NULL;
}

ActivityManagerHandler* ServiceHandler::activityManager()
{
	return NULL;
}

ActivityTaskManagerHandler* ServiceHandler::activityTaskManager(){
	return NULL;
}

PackageManagerHandler* ServiceHandler::packageManager()
{
	return NULL;
}

};

