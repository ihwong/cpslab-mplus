#ifndef MOBILEPLUS_REMOTEBINDERMANAGER_H
#define MOBILEPLUS_REMOTEBINDERMANAGER_H

#include <unordered_map>
#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/BpBinder.h>

#include <messaging/Semaphore.h>
#include <mobileplus/BnRemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>
#include <mobileplus/ServiceThread.h>
#include <mobileplus/RemoteProxy.h>
#include <mobileplus/RemoteActivityManager.h>
#include <mobileplus/RemoteActivityTaskManager.h>
#include <mobileplus/RemotePackageManager.h>

namespace android {
	
	class SharingElem;
	class ServiceThread;

  class RemoteBinderManager : public BnRemoteBinderManager {
  public:
    static void instantiate(const char *ipAddr);
	void initBasicServices();
	void initSharingMap();

	int matchWithSharingMap(String16* results, String16 callingPackageName, int code, int resultSize=-1);

	void registerServiceThread(int handle, ServiceThread* serviceThread);
	void eraseServiceThread(int handle);
	ServiceThread* findServiceThread(int handle); 
	
	void registerRemoteProxy(int handle, RemoteProxy* remoteProxy);
	void eraseRemoteProxy(int handle);
	RemoteProxy* findRemoteProxy(int handle);

	static int32_t getBinderHandle(Parcel& parcel);
	String16 getPackagesForUid(int32_t uid);

	int resolveActivity(const Parcel &data, String16** results);
	int resolveService(const Parcel &data, String16** results);

	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
	void handleParcel(int32_t sock, const char* callerName, const char* filePath, int32_t handle, uint32_t code, uint32_t flags, Parcel *parcel);

    ConnectionManager mConnectionManager;
	sp<IServiceManager> mServiceManager;
	sp<IBinder>	mActivityManager;
	sp<IBinder> mActivityTaskManager;
	sp<IBinder>	mPackageManager;
	
	int32_t mSmHandle;
	int32_t	mAmHandle;
	int32_t mATMHandle;
	int32_t	mPmHandle;
	int32_t mRemoteSmHandle;
	int32_t	mRemoteAmHandle;
	int32_t mRemoteATMHandle;
	int32_t	mRemotePmHandle;

	unordered_map<int, ServiceThread*> mServiceThreadMap;
	unordered_map<int, RemoteProxy*> mRemoteProxyMap;
	unordered_map<int, int> mRemoteUidMap;
	unordered_map<string, vector<SharingElem>*> mSharingMap;
	
	Semaphore mSema;

  private:
    RemoteBinderManager();
    RemoteBinderManager(const char *ipAddr);
    ~RemoteBinderManager();

  };

class SharingElem
{
public:
	SharingElem(string packageName, int type){
		mPackageName = packageName;
		mType = type;
	}
	~SharingElem() {}

	string mPackageName;
	int mType;
};

}; // end of namespace android

#endif
