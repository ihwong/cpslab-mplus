#ifndef MOBILEPLUS_REMOTEPACKAGEMANAGER_H
#define MOBILEPLUS_REMOTEPACKAGEMANAGER_H

#include <mobileplus/RemoteProxy.h>

namespace android {

class RemotePackageManager : public RemoteProxy {

public:
	RemotePackageManager(RemoteBinderManager* remoteBinderManager, ConnectionManager * connectionManager, int32_t sock, int32_t handle) : RemoteProxy(remoteBinderManager, connectionManager, sock, handle){
		mInterface = "android.content.pm.IPackageManager";
	}
	~RemotePackageManager() {}
	virtual RemotePackageManager* packageManager();

	virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
	int32_t remoteRegisterApp(String16 packageName, int32_t type);
	bool isEqualPkgName(char* dirName, const char* pkgName);
private:
	const char *mInterface;


};

};


#endif
