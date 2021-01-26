#ifndef MOBILEPLUS_SERVICEHANDLER_H
#define MOBILEPLUS_SERVICEHANDLER_H

#include <cutils/ashmem.h>
#include <binder/BpBinder.h>
#include <binder/Parcel.h>
#include <messaging/Handler.h>
#include <messaging/Message.h>
#include <messaging/Looper.h>
#include <mobileplus/UDSSenderThread.h>
#include <mobileplus/Global.h>
#include <mobileplus/ConnectionManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/RemoteProxy.h>

using namespace std;
using namespace android::messaging;

namespace android {

class ServiceManagerHandler;
class ActivityManagerHandler;
class ActivityTaskManagerHandler;
class PackageManagerHandler;
class RemoteBinderManager;
class ConnectionManager;

class ServiceHandler : public Handler
{
	friend class ServiceManagerHandler;
	friend class ActivityManagerHandler;
	friend class ActivityTaskManagerHandler;
	friend class PackageManagerHandler;

public:
	ServiceHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t handle);
	~ServiceHandler() {}
	virtual void handleMessage(Message& message);
	
	void serviceCall(int32_t sock, int32_t code, Parcel& data, Parcel* reply, uint32_t flags);
 	int getFdType(int fd);
	void handleBinderNode(int32_t type, Parcel& parcel, Parcel* remoteParcel=NULL, int32_t sock=0);
	
	virtual ServiceManagerHandler* serviceManager();
	virtual ActivityManagerHandler* activityManager();
	virtual ActivityTaskManagerHandler* activityTaskManager();
	virtual PackageManagerHandler* packageManager();

private:
	RemoteBinderManager* mRemoteBinderManager;
	ConnectionManager* mConnectionManager;
	int32_t	mHandle;
	UDSSenderThread* mUDSSenderThread;		// Assuming UDSSenderThread per ServiceThread


};

};

#endif
