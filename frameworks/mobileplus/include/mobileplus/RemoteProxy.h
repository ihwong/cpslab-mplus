#ifndef ANDROID_REMOTESPROXY_H
#define ANDROID_REMOTESPROXY_H

#include <unordered_map>
#include <binder/BpBinder.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>

#include <mobileplus/Global.h>
#include <mobileplus/BnRemoteProxy.h>
#include <mobileplus/UDSReceiverThread.h>
#include <messaging/Semaphore.h>

using namespace std;
using namespace android::messaging;

namespace android {

	class RemoteServiceManager;
	class RemoteActivityManager;
	class RemoteActivityTaskManager;
	class RemotePackageManager;
	class RemoteBinderManager;
	class ConnectionManager;
 
class RemoteProxy : public BnRemoteProxy{

	friend class RemoteServiceManager;
	friend class RemoteActivityManager;
	friend class RemoteActivityTaskManager;
	friend class RemotePackageManager;

public:
	RemoteProxy(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t sock, int32_t handle);
	~RemoteProxy() {}
	virtual status_t onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags);
	void handleBinderNode(int32_t type, Parcel& parcel, Parcel* remoteParcel = NULL);
	virtual RemoteServiceManager* serviceManager();
	virtual RemoteActivityManager* activityManager();
	virtual RemoteActivityTaskManager* activityTaskManager();
	virtual RemotePackageManager* packageManager();

	void wait();
	void signal();
	void setReply(Parcel *reply);
	Parcel* getReply();
	bool existReply();

	sp<IBinder> mBinderNode;
	int32_t mHandle;
	RemoteBinderManager *mRemoteBinderManager;
	ConnectionManager *mConnectionManager;
	int32_t mSock;
	Semaphore mSema;
	Mutex mTransLock;
	Parcel *mReply;
	UDSReceiverThread* mUDSReceiverThread;		// Assuming UDSReceiverThread per ServiceThread

};

};


#endif
