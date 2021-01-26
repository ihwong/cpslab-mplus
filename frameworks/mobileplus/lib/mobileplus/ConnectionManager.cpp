#define LOG_TAG "ConnectionManager.cpp"

#include <mobileplus/ConnectionManager.h>
#include <mobileplus/ListenThread.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/RemoteServiceManager.h>
#include <mobileplus/RemoteActivityManager.h>
//#include <mobileplus/RemotePackageManager.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cutils/ashmem.h>

namespace android {

void ConnectionManager::runSenderThread()
{
	ALOG(LOG_INFO, LOG_TAG, "runSenderThread() is called");
	mSenderThread = new SenderThread();
	mSenderThread->run("SenderThread");
}

void ConnectionManager::runListenThread()
{
	ALOG(LOG_INFO, LOG_TAG, "runListenThread() is called");
	mListenThread = new ListenThread(this);
	mListenThread->run("ListenThread");
}

void ConnectionManager::connectToServer(const char* ipAddr)
{
	ALOG(LOG_INFO, LOG_TAG, "connectToServer(), ipAddr = %s", ipAddr);
	int32_t sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
	serverAddr.sin_port = htons(LISTEN_PORT);
	while(1) {
		if (connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
			ALOG(LOG_INFO, LOG_TAG, "connect() is failed with err %d.", errno);
			sleep(1);
			continue;
		}
		else {
			ALOG(LOG_INFO, LOG_TAG, "connected to %s with sock %d.", ipAddr, sock);
			break;
		}
	}
	// naive implementation
	mServerSock = sock;
	mListenThread->mSocks[mServerSock].fd = mServerSock;
	mListenThread->mSocks[mServerSock].events = POLLIN;
	isConnected = true;

	sendBasicServiceHandle(mServerSock);
}

void ConnectionManager::sendBasicServiceHandle(int32_t sock)
{
	ALOG(LOG_INFO, LOG_TAG, "sendBasicServiceHandle(), sock = %d, serviceManagerHandle = %d, activityManagerHandle = %d, activityTaskManagerHandle = %d, packageManagerHandle = %d", sock, mRemoteBinderManager->mSmHandle, mRemoteBinderManager->mAmHandle, mRemoteBinderManager->mATMHandle, mRemoteBinderManager->mPmHandle);
	int8_t *buffer = new int8_t[TYPE_0_MESSAGE_LEN], *ptr = buffer;
	int8_t type = TYPE_0_MESSAGE; 
	int32_t smHandle = htonl(mRemoteBinderManager->mSmHandle);
	int32_t amHandle = htonl(mRemoteBinderManager->mAmHandle);
	int32_t atmHandle= htonl(mRemoteBinderManager->mATMHandle);
	int32_t pmHandle = htonl(mRemoteBinderManager->mPmHandle);

	memset(buffer, 0, TYPE_0_MESSAGE_LEN);
	buffer[0] = type;
	ptr += sizeof(int8_t);

	memcpy(ptr, &smHandle, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(ptr, &amHandle, sizeof(int32_t));
	ptr += sizeof(int32_t);
	
	memcpy(ptr, &atmHandle, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(ptr, &pmHandle, sizeof(int32_t));
	ptr += sizeof(int32_t);

	SenderHandler *handler = mSenderThread->getHandler();
	Message& msg = handler->obtainMessage(sock, TYPE_0_MESSAGE_LEN, 0);
	msg.obj = buffer;
	handler->sendMessage(msg);
}

void ConnectionManager::sendParcel(int32_t sock, const char *callerName, const char* filePath, int32_t handle, uint32_t code, uint32_t flags, const Parcel& parcel)
{
	int8_t type = TYPE_2_MESSAGE;
	int32_t callerNameLen = (callerName == NULL)? 0 : strlen(callerName), filePathLen = (filePath == NULL)? 0 : strlen(filePath);
	size_t parcelSize = parcel.dataSize(), refCount = parcel.objectsCount();
	const uint8_t *parcelData = parcel.data();
	const size_t *refData = (size_t*)parcel.objects();
	int32_t msgLen = TYPE_2_MESSAGE_LEN + callerNameLen + parcelSize + (refCount * sizeof(size_t)) + filePathLen;
	int8_t *buffer = new int8_t[msgLen], *ptr = buffer;
	
	ALOG(LOG_INFO, LOG_TAG, "sendParcel(msgLen= %d), sock = %d, parcel = %p", msgLen, sock, &parcel);
	ALOG(LOG_INFO, LOG_TAG, "callerName(%d) %s, filePath(%d) %s, handle %d, code %d, flags %d, parcel(%zu), refCount(%zu)", callerNameLen, callerName, filePathLen, filePath, handle, code, flags, parcelSize, refCount);

	memset(buffer, 0, msgLen);
	buffer[0] = type;
	ptr += sizeof(int8_t);

	
	msgLen = htonl(msgLen);
	memcpy(ptr, &msgLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	
	callerNameLen = htonl(callerNameLen);
	memcpy(ptr, &callerNameLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	if (callerName) {
		memcpy(ptr, callerName, ntohl(callerNameLen));
		ptr += ntohl(callerNameLen);
	}

	
	filePathLen = htonl(filePathLen);
	memcpy(ptr, &filePathLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	if (filePath){
		memcpy(ptr, filePath, ntohl(filePathLen));
		ptr += ntohl(filePathLen);
	}

	handle = htonl(handle);
	memcpy(ptr, &handle, sizeof(int32_t));
	ptr += sizeof(int32_t);


	code = htonl(code);
	memcpy(ptr, &code, sizeof(uint32_t));
	ptr += sizeof(uint32_t);


	flags = htonl(flags);
	memcpy(ptr, &flags, sizeof(uint32_t));
	ptr += sizeof(uint32_t);


	parcelSize = htonl(parcelSize);
	memcpy(ptr, &parcelSize, sizeof(size_t));
	ptr += sizeof(size_t);


	memcpy(ptr, parcelData, ntohl(parcelSize));
	ptr += ntohl(parcelSize);


	refCount = htonl(refCount);
	memcpy(ptr, &refCount, sizeof(size_t));
	ptr += sizeof(size_t);

	if (ntohl(refCount)) {
		memcpy(ptr, refData, ntohl(refCount) * sizeof(size_t));
		ptr += (ntohl(refCount) * sizeof(size_t));
	}

	SenderHandler *handler = mSenderThread->getHandler();
	Message& msg = handler->obtainMessage(sock, ntohl(msgLen), 0);
	msg.obj = buffer;
	handler->sendMessage(msg);	
	
}

void ConnectionManager::sendReply(int32_t sock, int32_t handle, Parcel &reply){

	int8_t type = TYPE_3_MESSAGE;
	size_t parcelSize = reply.dataSize(), refCount = reply.objectsCount();
	const uint8_t *parcelData = reply.data();
	const size_t *refData = (size_t*) reply.objects();
	int32_t msgLen = TYPE_3_MESSAGE_LEN + parcelSize + (refCount*sizeof(size_t));
	int8_t *buffer = new int8_t[msgLen], *ptr = buffer;
	
	ALOG(LOG_INFO, LOG_TAG, "sendReply(), sock %d, handle %d, parcelSize %zu refCount %zu reply %p", sock, handle, parcelSize, refCount, &reply);


	memset(buffer, 0, msgLen);
	buffer[0] = type;
	ptr += sizeof(int8_t);

	msgLen = htonl(msgLen);
	memcpy(ptr, &msgLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	handle = htonl(handle);
	memcpy(ptr, &handle, sizeof(int32_t));
	ptr += sizeof(int32_t);

	parcelSize = htonl(parcelSize);
	memcpy(ptr, &parcelSize, sizeof(size_t));
	ptr += sizeof(size_t);

	memcpy(ptr, parcelData, ntohl(parcelSize));
	ptr += ntohl(parcelSize);

	refCount = htonl(refCount);
	memcpy(ptr, &refCount, sizeof(size_t));
	ptr += sizeof(size_t);

	if (ntohl(refCount)) {
		memcpy(ptr, refData, ntohl(refCount) * sizeof(size_t));
		ptr += (ntohl(refCount) * sizeof(size_t));
	}
	

	SenderHandler *handler = mSenderThread->getHandler();
	Message& msg = handler->obtainMessage(sock, ntohl(msgLen), 0);
	msg.obj = buffer;
	handler->sendMessage(msg);

}

void ConnectionManager::sendFile(int32_t sock, uint32_t fileSize, int32_t fd, const char* fileName, int8_t fileType, const char* calleeName)
{
	ALOG(LOG_INFO, LOG_TAG,"sendFile(), sock %d, fileSize %d, fd %d, fileName %s, fileType %d", sock, fileSize, fd, fileName, fileType);
	int8_t type = TYPE_1_MESSAGE;
	int32_t fileNameLen = strlen(fileName), calleeNameLen = (calleeName == NULL) ? 0 : strlen(calleeName);
	int32_t msgLen = TYPE_1_MESSAGE_LEN + fileNameLen + fileSize + calleeNameLen;
	int8_t *buffer = new int8_t[msgLen], *ptr = buffer;
	int32_t bytes = 0;

	memset(buffer, 0, msgLen);
	buffer[0] = type;
	ptr += sizeof(int8_t);

	msgLen = htonl(msgLen);
	memcpy(ptr, &msgLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	fileSize = htonl(fileSize);
	memcpy(ptr, &fileSize, sizeof(uint32_t));
	ptr += sizeof(uint32_t);

	while((bytes = read(fd, ptr, 1024*128))>0)
		ptr += bytes;

	fileNameLen = htonl(fileNameLen);
	memcpy(ptr, &fileNameLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(ptr, fileName, ntohl(fileNameLen));
	ptr += ntohl(fileNameLen);

	*ptr = fileType;
	ptr += sizeof(int8_t);

	calleeNameLen = htonl(calleeNameLen);
	memcpy(ptr, &calleeNameLen, sizeof(int32_t));
	ptr += sizeof(int32_t);

	if (ntohl(calleeNameLen)){
		memcpy(ptr, calleeName, ntohl(calleeNameLen));
		ptr += ntohl(calleeNameLen);
	}

	SenderHandler *handler = mSenderThread->getHandler();
	Message& msg = handler->obtainMessage(sock, ntohl(msgLen), 0);
	msg.obj = buffer;
	handler->sendMessage(msg);
}





bool ConnectionManager::receiveMessage(int32_t sock){
	int8_t type = -1;
	recv(sock, &type, sizeof(int8_t), 0);

	ALOG(LOG_INFO, LOG_TAG, "receiveMessage type %d", type);

	if(type==1)
			return false;
	else if(type ==  TYPE_0_MESSAGE)
			receiveBasicServiceHandle(sock);
	else if(type == TYPE_1_MESSAGE)
			receiveFile(sock);
	else if(type == TYPE_2_MESSAGE)
			receiveParcel(sock);
	else if (type == TYPE_3_MESSAGE)
			receiveReply(sock);
	
	return true;	
}

void ConnectionManager::receiveBasicServiceHandle(int32_t sock){
	ALOG(LOG_INFO, LOG_TAG, "receiveBasicServiceHandle(), receive a TYPE_0_MESSAGE");	

	int32_t bufferLen = TYPE_0_MESSAGE_LEN -1;
	int8_t buffer[bufferLen], *ptr = buffer;
	memset(buffer, 0, bufferLen);
	
	int32_t RsmHandle=0, RamHandle=0, RatmHandle=0, RpmHandle=0;	// Remote Basic Service Handles

	recv(sock, buffer, bufferLen, MSG_WAITALL);
	
	memcpy(&RsmHandle, ptr, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(&RamHandle, ptr, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(&RatmHandle, ptr, sizeof(int32_t));
	ptr += sizeof(int32_t);

	memcpy(&RpmHandle, ptr, sizeof(int32_t));
	ptr += sizeof(int32_t);

	RsmHandle = ntohl(RsmHandle);
	RamHandle = ntohl(RamHandle);
	RatmHandle = ntohl(RatmHandle);
	RpmHandle = ntohl(RpmHandle);

	mRemoteBinderManager->mRemoteSmHandle = RsmHandle;
	mRemoteBinderManager->mRemoteAmHandle = RamHandle;
	mRemoteBinderManager->mRemoteATMHandle = RatmHandle;
	mRemoteBinderManager->mRemotePmHandle = RpmHandle;

	mRemoteBinderManager->registerRemoteProxy(RsmHandle, new RemoteServiceManager(mRemoteBinderManager, this, sock, RsmHandle));
	mRemoteBinderManager->registerRemoteProxy(RamHandle, new RemoteActivityManager(mRemoteBinderManager, this, sock, RamHandle));
	mRemoteBinderManager->registerRemoteProxy(RatmHandle, new RemoteActivityTaskManager(mRemoteBinderManager, this, sock, RatmHandle));
	mRemoteBinderManager->registerRemoteProxy(RpmHandle, new RemotePackageManager(mRemoteBinderManager, this, sock, RpmHandle));
	
	
	ALOG(LOG_INFO, LOG_TAG, "receiveBasicServiceHandle(), smHandle=%d, amHandle=%d, atmHandle=%d, pmHandle=%d", RsmHandle, RamHandle, RatmHandle, RpmHandle);	


	if(!isConnected){ // server side
		isConnected = true;
		sendBasicServiceHandle(sock);
	}
	
}

void ConnectionManager::receiveParcel(int32_t sock)
{
	int32_t msgLen= 0, bufferLen= 0, callerNameLen= 0, filePathLen= 0, handle= 0;
	uint32_t code= 0, flags= 0;
	int8_t *buffer=nullptr, *ptr=nullptr;
	size_t parcelSize= 0, refCount= 0;
	uint8_t *parcelData= nullptr;
	size_t *refData= nullptr;
	char *callerName=NULL, *filePath=NULL;

	recv(sock, &msgLen, sizeof(int32_t), 0);
	msgLen = ntohl(msgLen);
	bufferLen = msgLen-4-1;
	buffer = new int8_t[bufferLen];
	memset(buffer, 0, bufferLen);
	ptr = buffer;

	int32_t recvLen = recv(sock, buffer, bufferLen, MSG_WAITALL);
	ALOG(LOG_INFO, LOG_TAG, "receiveParcel(), recvLen= %d", recvLen);


	memcpy(&callerNameLen, ptr, sizeof(int32_t));
	callerNameLen = ntohl(callerNameLen);
	ptr += sizeof(int32_t);

	if (callerNameLen != 0){
		callerName = new char[callerNameLen + 1];
		memset(callerName, '\0', callerNameLen+1);
		memcpy(callerName, ptr, callerNameLen);
		ptr += callerNameLen;
	}

	memcpy(&filePathLen, ptr, sizeof(int32_t));
	filePathLen = ntohl(filePathLen);
	ptr += sizeof(int32_t);

	if(filePathLen != 0){
		filePath = new char[filePathLen+1];
		memset(filePath, '\0', filePathLen +1);
		memcpy(filePath, ptr, filePathLen);
		ptr += filePathLen;
	}

	memcpy(&handle, ptr, sizeof(int32_t));
	handle = ntohl(handle);
	ptr += sizeof(int32_t);

	memcpy(&code, ptr, sizeof(uint32_t));
	code = ntohl(code);
	ptr += sizeof(int32_t);

	memcpy(&flags, ptr, sizeof(uint32_t));
	flags = ntohl(flags);
	ptr += sizeof(uint32_t);

	memcpy(&parcelSize, ptr, sizeof(size_t));
	parcelSize = ntohl(parcelSize);
	ptr += sizeof(size_t);

	parcelData = new uint8_t[parcelSize];
	memset(parcelData, 0, parcelSize);
	memcpy(parcelData, ptr, parcelSize);
	ptr += parcelSize;

	memcpy(&refCount, ptr, sizeof(size_t));
	refCount = ntohl(refCount);
	ptr += sizeof(size_t);

	if(refCount) {
		refData = new size_t[refCount];
		memset(refData, 0, refCount*sizeof(size_t));
		memcpy(refData, ptr, refCount*sizeof(size_t));
		ptr += (refCount * sizeof(size_t));
	}	
	
	ALOG(LOG_INFO, LOG_TAG, "callerName(%d) %s, filePath(%d) %s, handle %d, code %d, flags %d, parcel(%zu), refCount(%zu)", callerNameLen, callerName, filePathLen, filePath, handle, code, flags, parcelSize, refCount);

	for (int i=0; i<refCount; i++){
		ALOG(LOG_INFO, LOG_TAG, "objects[%d] : %zu", i, refData[i]);
	}


	Parcel parcel;
	parcel.setData(parcelData, parcelSize);
	parcel.setObjects(refData, refCount);  // 
	parcel.print(alog);
	parcel.setDataPosition(0);
	mRemoteBinderManager->handleParcel(sock, callerName, filePath, handle, code, flags, &parcel);


	//Parcel *parcel = new Parcel();
	//parcel->setData(parcelData, parcelSize);
	//if(refCount)
	//		parcel->setObjects(refData, refCount);



	delete[] callerName;
	delete[] filePath;
	delete[] parcelData;
	delete[] refData;
	delete[] buffer;
}

void ConnectionManager::receiveReply(int32_t sock){

	int32_t msgLen=0, bufferLen=0, handle=0;
	int8_t *buffer=NULL, *ptr=NULL;
	size_t parcelSize= 0, refCount= 0;
	uint8_t *parcelData = NULL;
	size_t *refData = NULL;

	recv(sock, &msgLen, sizeof(int32_t), 0);
	msgLen = ntohl(msgLen);
	bufferLen = msgLen-5;
	buffer = new int8_t [bufferLen];
	memset(buffer, 0, bufferLen);
	ptr = buffer;

	int32_t recvLen = recv(sock, buffer, bufferLen, MSG_WAITALL);

	memcpy(&handle, ptr, sizeof(int32_t));
	handle = ntohl(handle);
	ptr += sizeof(int32_t);

	memcpy(&parcelSize, ptr, sizeof(size_t));
	parcelSize = ntohl(parcelSize);
	ptr += sizeof(size_t);

	parcelData = new uint8_t[parcelSize];
	memset(parcelData, 0, parcelSize);
	memcpy(parcelData, ptr, parcelSize);
	ptr += parcelSize;

	memcpy(&refCount, ptr, sizeof(size_t));
	refCount = ntohl(refCount);
	ptr += sizeof(size_t);

	if (refCount) {
		refData = new size_t[refCount];
		memset(refData, 0, refCount * sizeof(size_t));
		memcpy(refData, ptr, refCount * sizeof(size_t)); 
		ptr += (refCount * sizeof(size_t));
	}

	RemoteProxy *remoteProxy = mRemoteBinderManager->mRemoteProxyMap[handle];
	ALOG(LOG_INFO, LOG_TAG, "receiveReply(), receive (msgLen %d, recv %d), handle %d, parcelSize %zu refCount %zu",msgLen, recvLen, handle, parcelSize, refCount);

	Parcel* reply;
	// reply
	if (remoteProxy->existReply()){
		reply = remoteProxy->getReply();
	}
	else {
		reply = new Parcel();
		remoteProxy->setReply(reply);
	}
		
	if(parcelSize > 0){
		reply->setData(parcelData, parcelSize);
		if (refCount >0 ){
			reply->setObjects(refData, refCount);
		}
	}
	remoteProxy->signal();
	delete[] parcelData;
	delete[] refData;
	delete[] buffer;
	
}

void ConnectionManager::receiveFile(int32_t sock)
{
	int32_t msgLen=0, fileNameLen=0, bufferLen=0, calleeNameLen=0;
	uint32_t fileSize=0;
	int8_t *buffer=NULL, *ptr=NULL, *filePos=NULL, fileType=0;
	char *fileName=NULL, *calleeName=NULL;
	
	recv(sock, &msgLen, sizeof(int32_t), 0);
	msgLen = ntohl(msgLen);
	bufferLen = msgLen - 5;
	buffer = new int8_t[bufferLen];
	memset(buffer, 0, bufferLen);
	ptr = buffer;

	int32_t recvLen = recv(sock, buffer, bufferLen, MSG_WAITALL);
	ALOG(LOG_INFO, LOG_TAG, "receiveFile(), receive a message (length =%d), recvLen=%d", msgLen, recvLen);

	memcpy(&fileSize, ptr, sizeof(uint32_t));
	fileSize = ntohl(fileSize);
	ptr += sizeof(uint32_t);
	filePos = ptr;
	ptr += fileSize;

	memcpy(&fileNameLen, ptr, sizeof(int32_t));
	fileNameLen = ntohl(fileNameLen);
	ptr += sizeof(int32_t);
	
	fileName = new char[fileNameLen + 1];
	memset(fileName, 0, fileNameLen + 1);
	memcpy(fileName, ptr, fileNameLen);
	fileName[fileNameLen] = '\0';
	ptr += fileNameLen;

	fileType = *ptr;
	ptr += sizeof(int8_t);

	memcpy(&calleeNameLen, ptr, sizeof(int32_t));
	calleeNameLen = ntohl(calleeNameLen);
	ptr += sizeof(int32_t);

	if (calleeNameLen) {
		calleeName = new char[calleeNameLen + 1];
		memset(calleeName, 0, calleeNameLen + 1);
		memcpy(calleeName, ptr, calleeNameLen);
		calleeName[calleeNameLen] = '\0';
		ptr += calleeNameLen;
	}

	switch (fileType) {
		case FileType::APK_FILE_TYPE: {
			char filePath[500] = "/data/app/com.mobileplus.remoteapps/";
			mkdir(filePath, 0777);
			strcat(filePath, fileName);
			strcat(filePath, "/");
			mkdir(filePath, 0777);
			strcat(filePath, fileName);
			strcat(filePath, ".apk");
			int32_t fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			write(fd, filePos, fileSize);
			close(fd);
			break;
		}
		case FileType::PDF_FILE_TYPE: {
			char filePath[500] = "/data/data/";
			strcat(filePath, calleeName);
			strcat(filePath, "/files/");
			strcat(filePath, fileName);
			strcat(filePath, ".pdf");
			ALOG(LOG_INFO, LOG_TAG, "receiveFile(), filePath = %s", filePath);
			int32_t fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			write(fd, filePos, fileSize);
			close(fd);
			break;
		}
		case FileType::IMAGE_FILE_TYPE: {
			char filePath[500] = "/data/data/";
			strcat(filePath, calleeName);
			strcat(filePath, "/files/");
			strcat(filePath, fileName);
			strcat(filePath, ".jpg");
			ALOG(LOG_INFO, LOG_TAG,"receiveFile(), filePath = %s", filePath);
			int32_t fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			write(fd, filePos, fileSize);
			close(fd);
			break;
		}
		default: 
			break;
	}

	delete[] fileName;
	delete[] calleeName;
	delete[] buffer;
}





}; 
