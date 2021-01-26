#define LOG_TAG "ActivityTaskManagerHandler.cpp"

#include <mobileplus/ActivityTaskManagerHandler.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>
#include <mobileplus/NativeIntent.h>
#include <binder/IPCThreadState.h>

namespace android {

ActivityTaskManagerHandler* ActivityTaskManagerHandler::activityTaskManager(){
	return this;
}

void ActivityTaskManagerHandler::handleMessage(Message& message){
	int32_t code= message.what;
	int32_t sock = message.arg1;
	uint32_t flags = message.arg2;

	Parcel* parcel = (Parcel*) message.obj, *reply=(flags & REPLY_OFF) ? NULL : new Parcel();
	String16* filePath = (String16*) message.obj2;

	ALOG(LOG_INFO, LOG_TAG, "handleMessage(), code %d, parcel %p", code, parcel);

	switch (code){
		case SharingType::REMOTE_START_ACTIVITY_TRANSACTION: {
			startActivity(*parcel, reply, flags, filePath);
			mConnectionManager->sendReply(sock, mHandle, *reply);
			break;
		}
		case SharingType::REMOTE_SCHEDULE_SEND_RESULT_TRANSACTION: {
			scheduleSendResult(*parcel, flags); 
			break;														   
		}
	}
	
	parcel->freeDataNoInit();
	delete reply;
	delete filePath;
	
}


void ActivityTaskManagerHandler::startActivity(Parcel& data, Parcel* reply, uint32_t flags, String16* filePath){

	ALOG(LOG_INFO, LOG_TAG, "startActivity() with data %p, reply %p", &data, reply);
	int32_t atmHandle = mRemoteBinderManager->mRemoteATMHandle;
	RemoteProxy* rATM = mRemoteBinderManager->findRemoteProxy(atmHandle);
	const size_t* objects = (size_t*) data.objects();
	size_t objectCount = data.objectsCount(), startPos = 0, len= 0;

	//change binder objects
	Parcel temp;
	for (int i=0; i<objectCount; i++){
		len = objects[i] - startPos;
		temp.appendFrom(&data, startPos, len);
		temp.writeStrongBinder(rATM->mBinderNode);

		data.setDataPosition(objects[i]);
		data.readStrongBinder();
		startPos = data.dataPosition();
	}
	temp.appendFrom(&data, startPos, data.dataSize()-startPos);

	//get Intent
	data.setDataPosition(objects[0]);
	data.readStrongBinder();
	data.readString16();
	NativeIntent *intent = new NativeIntent();
	if (data.readInt32() != 0){
		intent->readFromParcel(data);
	}

	// Change Uri
	Parcel request;
	if(intent->hasUri() && filePath){
		// Naive implementation: I assume all the URIs are same.
		int uriNum = intent->mUriNum;
		int bundleLengthPos = intent->mBundleLengthPos;
		temp.setDataPosition(intent->mBundleLengthPos);
		int bundleLength = temp.readInt32();
		request.appendFrom(&temp, 0, intent->mStartPos[0]);
		for (int i = 0; i < uriNum; i++) {
			int endPos = intent->mEndPos[i];
			int nextStartPos = (i < (uriNum - 1))? intent->mStartPos[i + 1] : temp.dataSize();

			int uriStartPos = request.dataPosition();
			NativeIntent::writeUri(request, *filePath);
			int uriEndPos = request.dataPosition();
			request.appendFrom(&temp, endPos, nextStartPos - endPos);
			
			int oriUriLength = intent->mEndPos[i] - intent->mStartPos[i];
			int newUriLength = uriEndPos - uriStartPos;
			int diff = newUriLength - oriUriLength;
			if (intent->mBundleLengthPos != -1 && uriStartPos > intent->mBundleLengthPos) {
				bundleLength += diff;
				request.setDataPosition(bundleLengthPos);
				request.writeInt32(bundleLength);
			}
			else {
				bundleLengthPos += diff;
			}
		}
	}
	else
		request.appendFrom(&temp, 0, temp.dataSize());
	
	request.print(alog);
	IPCThreadState::self()->transact(mHandle, ActivityTaskManager::TRANSACTION_REMOTE_START_ACTIVITY, request, reply, flags);
	
}	

void ActivityTaskManagerHandler::scheduleSendResult(Parcel& data, uint32_t flags)
{
	data.readInt32();
	data.readInt32();
	String16 interfaceName = data.readString16();
	int callerHandle = data.readInt32();
	size_t startPos = data.dataPosition();
	size_t len = data.dataSize() - startPos;
	sp<IBinder> resultTo = mResultToMap.find(callerHandle)->second;
	ALOG(LOG_INFO, LOG_TAG, "scheduleSendResult(), interfaceName=%s, callerHandle=%d, resultTo=%p", String8(interfaceName).string(), callerHandle, &resultTo);
	
	//create a new parcel
	Parcel request, reply;
	request.writeInterfaceToken(String16("android.app.IActivityTaskManager"));
	request.writeStrongBinder(resultTo);
	request.appendFrom(&data, startPos, len);
	IPCThreadState::self()->transact(mHandle, ActivityTaskManager::TRANSACTION_FINISH_VIRTUAL_ACTIVITY, request, &reply, flags);
	mResultToMap.erase(mResultToMap.find(callerHandle));
}

};
