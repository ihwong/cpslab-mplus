#include <mobileplus/PackageManagerHandler.h>

namespace android {

PackageManagerHandler* PackageManagerHandler::packageManager(){
	return this;
}

void PackageManagerHandler::handleMessage(Message &message){
	uint32_t code = message.what;
	int32_t sock = message.arg1;
	uint32_t flags = message.arg2;
	Parcel *parcel = (Parcel*) message.obj, *reply=(flags & REPLY_OFF) ? NULL : new Parcel();

	ALOG(LOG_INFO, LOG_TAG, "handleMessage(), code %d, parcel %p, sock %d", code, parcel, sock);
	/*
	switch (code){
		case PackagesManager::REMOTE_REGISTER_APP_TRANSACTION:{
			IPCThreadState::self()->transact(mHandle, code, *parcel, reply, flags);
			reply->readExceptionCode();
			int32_t ret = reply->readInt32();
			ALOG(LOG_INFO, LOG_TAG, "handleMessage, ret =%d", ret);
			mConnectionManager->sendReply(sock, mHandle, *reply);
			break; 
		}
	}*/

	parcel->freeDataNoInit();
	delete parcel;
	delete reply;
}

};
