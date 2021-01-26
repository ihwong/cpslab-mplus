#ifndef MOBILEPLUS_REMOTEACTIVITYMANAGER_H
#define MOBILEPLUS_REMOTEACTIVITYMANAGER_H

#include <unordered_map>

#include <mobileplus/RemoteProxy.h>

namespace android {

class RemoteActivityManager : public RemoteProxy
{

public:
	RemoteActivityManager(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t sock, int32_t handle) 
			: RemoteProxy(remoteBinderManager, connectionManager, sock, handle) {}
	~RemoteActivityManager() {}
	virtual RemoteActivityManager* activityManager();

	//virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
	void bindService(int32_t callingUid, String16 callerName, Parcel& data, Parcel* reply, uint32_t flags);


};


};

#endif
