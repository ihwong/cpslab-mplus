#ifndef MOBILEPLUS_PACKAGEMANAGERHANDLER_H
#define MOBILEPLUS_PACKAGEMANAGERHANDLER_H

#include <mobileplus/ServiceHandler.h>

using namespace android::messaging;

namespace android{

class PackageManagerHandler : public ServiceHandler {

public:
	PackageManagerHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager* connectionManager, int32_t handle): ServiceHandler(remoteBinderManager, connectionManager, handle){}
	~PackageManagerHandler() {}
	virtual PackageManagerHandler* packageManager();

	virtual void handleMessage(Message& message);

};

};

#endif
