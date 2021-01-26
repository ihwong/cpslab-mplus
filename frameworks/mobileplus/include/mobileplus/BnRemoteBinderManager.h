#ifndef MOBILEPLUS_BNREMOTEBINDERMANAGER_H
#define MOBILEPLUS_BNREMOTEBINDERMANAGER_H

#include <binder/Parcel.h>
#include <mobileplus/IRemoteBinderManager.h>

namespace android {

  class BnRemoteBinderManager : public BnInterface<IRemoteBinderManager> {
  public:
    // BBinder 클래스의 onTransact() 메서드 재정의
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
  };

}; // end of namespace android

#endif
