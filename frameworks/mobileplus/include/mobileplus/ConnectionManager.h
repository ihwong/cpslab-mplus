#ifndef MOBILEPLUS_CONNNECTIONMANAGER_H
#define MOBILEPLUS_CONNNECTIONMANAGER_H

#include <mobileplus/Global.h>
#include <mobileplus/IRemoteBinderManager.h>
#include <mobileplus/SenderThread.h>
#include <mobileplus/ListenThread.h>

namespace android {
	
class RemoteBinderManager;
class ListenThread;
class ConnectionManager 
{
	friend class RemoteBinderManager;
	friend class ListenThread;
public:
	ConnectionManager() {
		isConnected = false;
	}
	~ConnectionManager() {}
	void runSenderThread();
	void runListenThread();
	void connectToServer(const char* ipAddr);
	
	void sendBasicServiceHandle(int32_t sock);
	void sendParcel(int32_t sock, const char *callerName, const char* calleeName, int32_t handle, uint32_t code, uint32_t flags, const Parcel& parcel);
	void sendReply(int32_t sock, int32_t handle, Parcel &reply);
	void sendFile(int32_t sock, uint32_t fileSize, int32_t fd, const char *fileName, int8_t fileType, const char* calleeName = NULL);
	//void sendAshmem(int32_t sock, int32_t ashmemFd, uint32_t size);
	
	bool receiveMessage(int32_t sock);
	void receiveBasicServiceHandle(int32_t sock);
	void receiveParcel(int32_t sock);
	void receiveReply(int32_t sock);
	void receiveFile(int32_t sock);
	//void receiveAshmem(int32_t sock);
	
	RemoteBinderManager *mRemoteBinderManager;
	SenderThread *mSenderThread;
	sp<ListenThread> mListenThread;
	bool isConnected;	// temp variable
	int32_t mClientSock;	// temp variable
	int32_t mServerSock;	// temp variable
	const char* mClientIpAddr;
};

};
#endif
