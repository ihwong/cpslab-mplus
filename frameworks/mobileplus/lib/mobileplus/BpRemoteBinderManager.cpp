#include <binder/Parcel.h>
#include <mobileplus/BpRemoteBinderManager.h>

namespace android {

  BpRemoteBinderManager::BpRemoteBinderManager(const sp<IBinder>& impl) : BpInterface<IRemoteBinderManager>(impl) {
    // nothing here
  }
    
}; // end of namespace android
