#include <mobileplus/BpRemoteProxy.h>

namespace android {
		
BpRemoteProxy::BpRemoteProxy(const sp<IBinder>& impl)
		:BpInterface<IRemoteProxy>(impl) {}

}
