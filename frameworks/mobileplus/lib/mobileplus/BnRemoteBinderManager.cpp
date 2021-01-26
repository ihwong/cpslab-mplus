#define LOG_TAG "BnRemoteBinderManager.cpp"

#include <mobileplus/BnRemoteBinderManager.h>
#include <binder/Parcel.h>

#define UNUSED(x) (void)(x)

namespace android {

  status_t BnRemoteBinderManager::onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags) {

	UNUSED(code);
	UNUSED(data);
	UNUSED(reply);
	UNUSED(flags);
	
    return NO_ERROR;
    
  }
  
}; // end of namespace android
