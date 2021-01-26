#include <mobileplus/BnRemoteProxy.h>

#define UNUSED(x) (void)(x)

namespace android {

status_t BnRemoteProxy::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){
	
	UNUSED(code);
	UNUSED(data);
	UNUSED(reply);
	UNUSED(flags);

	return NO_ERROR;
}
	
};
