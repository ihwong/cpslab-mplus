#ifndef MOBILEPLUS_REMOTEACTIVITYTASKMANAGER_H
#define MOBILEPLUS_REMOTEACTIVITYTASKMANAGER_H

#include <mobileplus/RemoteProxy.h>
#include <mobileplus/NativeIntent.h>
#include <unordered_map>

namespace android {

class RemoteActivityTaskManager : public RemoteProxy {

public:
	RemoteActivityTaskManager(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t sock, int32_t handle)
			: RemoteProxy(remoteBinderManager, connectionManager, sock, handle) {}

	~RemoteActivityTaskManager() {}
	virtual RemoteActivityTaskManager* activityTaskManager();
	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);



	void startActivity(int32_t callingUid, String16 callerName, String16 calleeName, Parcel& data, Parcel* reply, uint32_t flags);
	const char* sendFileForUri(Parcel& data, NativeIntent& intent, sp<IBinder> caller, String16 callerName, String16 calleeName);
	void scheduleSendResult(const Parcel& data, Parcel* reply, uint32_t flags);

	unordered_map<int, RemoteProxy*> mProviderConnectionMap;

};

};

#endif 
