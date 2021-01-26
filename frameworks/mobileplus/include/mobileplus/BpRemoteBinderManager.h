#ifndef MOBILEPLUS_BPREMOTEBINDERMANAGER_H
#define MOBILEPLUS_BPREMOTEBINDERMANAGER_H

#include <binder/Parcel.h>
#include <mobileplus/IRemoteBinderManager.h>

namespace android {
  class BpRemoteBinderManager : public BpInterface<IRemoteBinderManager> {
  public:
    BpRemoteBinderManager(const sp<IBinder>& impl);
  };
}; // end of namespace android

#endif
