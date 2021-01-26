#ifndef ANDROID_BNREMOTEPROXY_H
#define ANDROID_BNREMOTEPROXY_H

#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/Parcel.h>

#include <mobileplus/IRemoteProxy.h>

namespace android {

class BnRemoteProxy : public BnInterface<IRemoteProxy>
{
	public:
			virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags =0);
};
};



#endif
