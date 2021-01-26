#define LOG_TAG "RemoteActivityTaskManager.cpp"

#include <mobileplus/RemoteActivityTaskManager.h>
#include <mobileplus/ActivityTaskManagerHandler.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/RemotePackageManager.h>
#include <mobileplus/ConnectionManager.h>
#include <mobileplus/NativeIntent.h>


namespace android {

RemoteActivityTaskManager* RemoteActivityTaskManager::activityTaskManager(){
	return this;
}

status_t RemoteActivityTaskManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
	ALOG(LOG_INFO, LOG_TAG, "onTransact(), code %d, data %p, reply %p, flags %d", code, &data, reply, flags);
	
	switch (code){
		case SharingType::REMOTE_SCHEDULE_SEND_RESULT_TRANSACTION: {
			scheduleSendResult(data, reply, flags);
			break;														   
		}
	}
	
	return NO_ERROR;
}

void RemoteActivityTaskManager::startActivity(int32_t callingUid, String16 callerName, String16 calleeName, Parcel& data, Parcel *reply, uint32_t flags){
	
	data.setDataPosition(0);
	int objectsCount = data.objectsCount();
	const size_t* objects = (size_t*) data.objects();

	data.print(alog);
	ALOG(LOG_INFO, LOG_TAG, "startActivity with uid %d, callerName %s, calleeName %s, flags %d, data %p(objects=%d), reply %p", callingUid, String8(callerName).string(), String8(calleeName).string(), flags, &data, objectsCount, reply);

	
	data.setDataPosition(objects[objectsCount-1]);
	sp<IBinder> resultTo = data.readStrongBinder(); 	//resultTo
	String16 resultWho = data.readString16(); 			//resultWho
	int requestCode = data.readInt32(); 				//requestCode	

	//Parser the incoming parcel
	data.setDataPosition(objects[0]);  					//caller	
	sp<IBinder> caller = data.readStrongBinder();
	int32_t callerHandle = ((BpBinder*) caller->remoteBinder())->handle();
	String16 callingPackage = data.readString16(); 		//callingPackage;

	NativeIntent *intent = new NativeIntent();
	if (data.readInt32() != 0){
		intent->readFromParcel(data);
	}

	// store the needed information
	int atmHandle = mRemoteBinderManager->mATMHandle;
	ServiceThread* sThread = mRemoteBinderManager->findServiceThread(atmHandle);
	if (sThread){
		ServiceHandler* handler = sThread->getHandler();
		ActivityTaskManagerHandler* atmHandler = handler->activityTaskManager();
		atmHandler->mResultToMap[callerHandle] = resultTo;
	}
	
	// Get caller's componentName from ActivitTaskManager service
	Parcel query, answer;
	query.writeInterfaceToken(String16("android.app.IActivityTaskManager"));
	query.writeStrongBinder(resultTo);
	status_t status = IPCThreadState::self()->transact(
					mRemoteBinderManager->mATMHandle, ActivityTaskManager::TRANSACTION_GET_ACTIVITY_CLASS_FOR_TOKEN, query, &answer, REPLY_ON);
	answer.readExceptionCode();
	size_t startPos = answer.dataPosition();
	size_t len = answer.dataSize() - startPos;


	// Register the caller app into the server device
	unordered_map<int, int>* remoteUidMap = &(mRemoteBinderManager->mRemoteUidMap);
	int remoteUid = -1;

	if (remoteUidMap->size() != 0){
		unordered_map<int, int>::iterator itr = remoteUidMap->find(callingUid);
		if (itr != remoteUidMap->end()){
			remoteUid = itr->second;
		}
	} 
	if (remoteUid == -1){ // remoteUidMap doesn't have callingUid
		int32_t pmHandle = mRemoteBinderManager->mRemotePmHandle;
		RemotePackageManager* remotePm = mRemoteBinderManager->findRemoteProxy(pmHandle)->packageManager();
		
		//remoteUid = remotePm->remoteRegisterApp(callerName, ONLY_PACKAGE_INFO);
		UNUSED(remotePm);
		if (callerName == String16("com.instagram.android"))
			remoteUid = 10119; // instagram
		else 
			remoteUid = 10117; // toyapp_client
		
		remoteUidMap->insert(pair<int, int>(callingUid, remoteUid));

	}

	// Send a file if URI exists in the parcel
	const char* filePath = NULL;
	if (intent->hasUri())
		filePath = sendFileForUri(data, *intent, caller, callerName, calleeName);
	
	//write additional informations
	data.setDataPosition(data.dataSize());
	data.writeInt32(callerHandle);
	data.appendFrom(&answer, startPos, len);
	data.writeRemoteUid(remoteUid);

	data.setDataPosition(0);
	// Start the virtual activity	
	if ((!intent->hasUri() || filePath == NULL) && requestCode != -1){
		query.freeData();
		answer.freeData();
		query.writeInterfaceToken(String16("android.app.IActivityTaskManager"));
		query.writeStrongBinder(caller);
		query.writeStrongBinder(resultTo);
		query.writeInt32(requestCode);
		status = IPCThreadState::self()->transact(mRemoteBinderManager->mATMHandle, ActivityTaskManager::TRANSACTION_START_VIRTUAL_ACTIVITY, query, &answer, REPLY_ON);
	}

	//Send the parcel
	mConnectionManager->sendParcel(mSock, String8(callerName).string(), filePath, mHandle, SharingType::REMOTE_START_ACTIVITY_TRANSACTION, flags, data);

	delete filePath;	
	
	wait();
	reply->setDataPosition(0);
	mReply = NULL;
	
}

const char* RemoteActivityTaskManager::sendFileForUri(Parcel& data, NativeIntent& intent, sp<IBinder> caller, String16 callerName, String16 calleeName)
{
	// Naive implementation: I assume all the URIs are same.
	int fd = -1;
	int amHandle = mRemoteBinderManager->mAmHandle;

	// Get an autority from URI
	Uri* uri = &(intent.mUri[0]);
	int startPos = intent.mStartPos[0];
	int endPos = intent.mEndPos[0];
	String16 authority;
	if (uri->type == UriType::STRING_URI)
		authority = ((StringUri*)uri->uri)->getAuthority();
	else if (uri->type == UriType::HIERARCHICAL_URI) {
		int type = ((HierarchicalUri*)uri->uri)->authority.type;
		authority = (type != 2)? ((HierarchicalUri*)uri->uri)->authority.encoded : ((HierarchicalUri*)uri->uri)->authority.decoded;
	}

	ALOG(LOG_INFO, LOG_TAG, "sendFileForUri(), authority %s, intent.mType %s", String8(authority).string(), String8(intent.mType).string() );

	// Get a content provider
	Parcel query, answer;
	query.writeInterfaceToken(String16("android.app.IActivityManager"));
	query.writeStrongBinder(caller);
	query.writeString16(authority);
	query.writeInt32(0);
	query.writeInt32(0);
	IPCThreadState::self()->transact(amHandle, ActivityManager::TRANSACTION_GET_CONTENT_PROVIDER, query, &answer, 0);
	answer.readExceptionCode();
	int res = answer.readInt32();
	if (res) {
		const size_t* objects = (size_t*)answer.objects();
		answer.setDataPosition(objects[0]);
		sp<IBinder> contentProvider = answer.readStrongBinder();
		int32_t cpHandle = contentProvider->remoteBinder()->handle();

		// Get a file descriptor
		query.freeData();
		answer.freeData();
		query.writeInterfaceToken(String16("android.content.IContentProvider"));
		query.writeString16(callerName);
		query.appendFrom(&data, startPos, endPos - startPos);		//uri
		query.writeString16(String16("*/*")); 						//mimeType
		query.writeInt32(-1); 										//Bundle
		query.writeStrongBinder(NULL); 								//signal
		IPCThreadState::self()->transact(cpHandle, ContentProvider::OPEN_TYPED_ASSET_FILE_TRANSACTION, query, &answer, 0);
		answer.readExceptionCode();
		res = answer.readInt32();
		if (res) {
			objects = (size_t*)answer.objects();
			answer.setDataPosition(objects[0]);
			fd = answer.readFileDescriptor();
		}
	}

	if (fd == -1)
		return NULL;

	// send this file
	int fileSize = lseek(fd, 0, SEEK_END);
	int type = 0;
	char fileName[100];
	strcpy(fileName, String8(callerName).string());
	if (intent.mType == String16("application/pdf"))
		type = FileType::PDF_FILE_TYPE;
	else if (intent.mType == String16("image/*"))
		type = FileType::IMAGE_FILE_TYPE;
	lseek(fd, 0, SEEK_SET);
	mConnectionManager->sendFile(mSock, fileSize, fd, fileName, type, String8(calleeName).string());
	close(fd);

	// make up a file path
	char* filePath = new char[100];
	memset(filePath, 0, 100);
	strcpy(filePath, "/data/data/");
	strcat(filePath, String8(calleeName).string());
	strcat(filePath, "/files/");

//	strcat(filePath, "/sdcard/");

	strcat(filePath, String8(callerName).string());
	if (intent.mType == String16("application/pdf"))
		strcat(filePath, ".pdf");
	if (intent.mType == String16("image/*"))
		strcat(filePath, ".jpg");
	return filePath;
}

void RemoteActivityTaskManager::scheduleSendResult(const Parcel& data, Parcel* reply, uint32_t flags)
{
	data.print(alog);
	data.setDataPosition(0);
	data.readInt32();
	data.readInt32();
	
	String16 interfaceName = data.readString16();
	int callerHandle = data.readInt32();

	ALOG(LOG_INFO, LOG_TAG, "scheduleSendResult(), interfaceName= %s, callerHandle= %d",String8(interfaceName).string(), callerHandle);
	if (callerHandle != -1){
		mConnectionManager->sendParcel(mSock, NULL, NULL, mHandle, SharingType::REMOTE_SCHEDULE_SEND_RESULT_TRANSACTION, flags, data);
		data.setDataPosition(0);
	}

}


};
