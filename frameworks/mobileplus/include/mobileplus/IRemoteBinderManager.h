#ifndef MOBILEPLUS_IREMOTEBINDERMANAGER_H
#define MOBILEPLUS_IREMOTEBINDERMANAGER_H

#include <binder/IInterface.h>

namespace android {

  // 서비스 인터페이스인 IRemoteBinderManager 클래스 정의
  class IRemoteBinderManager : public IInterface {

  public:
    // 서비스 인터페이스 매크로
    DECLARE_META_INTERFACE(RemoteBinderManager);
    // 서비스 함수
    // virtual status_t helloWorld(const char *str) = 0;

  };
  
}; // end of namespace android

#endif
