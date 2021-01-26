#ifndef MOBILEPLUS_REMOTESERVICEMANAGER_H
#define MOBILEPLUS_REMOTESERVICEMANAGER_H

#include <mobileplus/RemoteProxy.h>

namespace android {

class RemoteServiceManager : public RemoteProxy
{
public:
	RemoteServiceManager(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t sock, int32_t handle) : RemoteProxy(remoteBinderManager, connectionManager, sock, handle) {}
	~RemoteServiceManager() {}
	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
	virtual RemoteServiceManager* serviceManager();
	void getService(int32_t callingUid, String16 callerName, String16 serviceName, Parcel& data, Parcel* reply, uint32_t flags);
private:
};

}; 

#endif
