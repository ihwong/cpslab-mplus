#ifndef ANDROID_BPREMOTEPROXY_H
#define ANDROID_BPREMOTEPROXY_H

#include <binder/Parcel.h>
#include <mobileplus/IRemoteProxy.h>

namespace android {

class BpRemoteProxy : public BpInterface<IRemoteProxy>{

public :
	BpRemoteProxy(const sp<IBinder>& impl);
};		

};

#endif

