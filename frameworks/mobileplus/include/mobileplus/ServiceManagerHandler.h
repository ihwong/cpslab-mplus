#ifndef MOBILEPLUS_SERVICEMANAGERHANDLER_H
#define MOBILEPLUS_SERVICEMANAGERHANDLER_H

#include <mobileplus/ServiceHandler.h>

using namespace android::messaging;

namespace android {

class ServiceManagerHandler : public ServiceHandler
{
public:
	ServiceManagerHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t handle) : ServiceHandler(remoteBinderManager, connectionManager, handle) {}
	~ServiceManagerHandler() {}
	virtual void handleMessage(Message& message);
	virtual ServiceManagerHandler* serviceManager();
	void getService(int32_t sock, Parcel& data, Parcel* reply, uint32_t flags);

private:
};

}; 
#endif

