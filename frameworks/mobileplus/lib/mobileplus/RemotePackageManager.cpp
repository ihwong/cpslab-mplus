#define LOG_TAG "RemotePackageManager.cpp"
#define PKG_INFO_ALL_FLAGS 	20831

#include <mobileplus/RemotePackageManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>

#include <fcntl.h>
#include <dirent.h>


namespace android {

status_t RemotePackageManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){
	return NO_ERROR;
}

RemotePackageManager* RemotePackageManager::packageManager()
{
	return this;
}

int32_t RemotePackageManager::remoteRegisterApp(String16 packageName, int32_t type)
{
	ALOG(LOG_INFO, LOG_TAG, "remoteRegisterApp() packageName %s, type %d", String8(packageName).string(), type);	

	return -1;
	/*
	if (type == APK_AND_PACKAGE_INFO){
		char fileName[100] = "data/app/";
		DIR *pDir = opendir("/data/app");
		dirent *pFile = nullptr;
		if (pDir) {
			while ((pFile = readdir(pDir)) != NULL){
				if (isEqualPkgName(pFile->d_name, String8(packageName).string()))
					break;
			}
			strcat(fileName, pFile->d_name);
			strcat(fileName, "/base.spk");

			int32_t fd = open(fileName, O_RDONLY);
			uint32_t fileSize = lseek(fd, 0, SEEK_END);
			lseek(fd, 0, SEEK_SET);
			mConnectionManager->sendFile(mSock, fileSize, fd, String8(packageName).string(), FileType::APK_FILE_TYPE);
			close(fd);
			closedir(pDir);

		}
	}

	Parcel query, answer;
	query.writeInterfaceToken(String16(mInterface));
	query.writeString16(packageName);
	query.writeInt32(PKG_INFO_ALL_FLAGS);
	query.writeInt32(0);
	status_t status = IPCThreadState::self()->transact(mRemoteBinderManager->mPmHandle, PackagesManager::GET_PACKAGE_INFO_TRANSACTION, query, &answer, 0);
	answer.readExceptionCode();
	int result = answer.readInt32();

	Parcel request; 
	request.writeInterfaceToken(String16(mInterface));
	request.writeString16(packageName);
	request.writeInt32(result);
	if(result){
		size_t packageInfoPos = answer.dataPosition();
		size_t packageInfoLen = answer.dataSize()-packageInfoPos;

		request.appendFrom(&answer, packageInfoPos, packageInfoLen);
	}
	request.writeInt32(type);
	
	mConnectionManager->sendParcel(mSock, String8(packageName).string(), NULL, mHandle, PackagesManager::REMOTE_REGISTER_APP_TRANSACTION, REPLY_ON, request);

	wait();
	mReply->readExceptionCode();
	int32_t remoteUid = mReply->readInt32();
	ALOG(LOG_INFO, LOG_TAG, "remoteRegisterApp(), remoteUid = %d", remoteUid);
	
	mReply->freeDataNoInit();
	mReply = nullptr;
	return remoteUid;	
	*/
}


bool RemotePackageManager::isEqualPkgName(char *dirName, const char *pkgName){
	const char* ptr1 = dirName, *ptr2 = pkgName;

	while (*ptr2 != '\0'){
		if (*ptr2 != *ptr1)
				return false;
	
		ptr1++;
		ptr2++;
	}

	return true;
}

};
