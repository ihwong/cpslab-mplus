#ifndef MOBILEPLUS_ACTIVITYMANAGERHANDLE_H
#define MOBILEPLUS_ACTIVITYMANAGERHANDLE_H

#include <unordered_map>
#include <mobileplus/ServiceHandler.h>

using namespace android::messaging;
using namespace std;

namespace android {

class ActivityManagerHandler : public ServiceHandler{

public :
		ActivityManagerHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t handle)
			: ServiceHandler(remoteBinderManager, connectionManager, handle){
			mInterface = String16("android.app.IActivityManager");
		}
		~ActivityManagerHandler() {}
		virtual ActivityManagerHandler* activityManager();

		virtual void handleMessage(Message& message);
		
		void bindService(int32_t sock, Parcel& data, Parcel* reply, uint32_t flags);


private : 
		String16 mInterface;
};

};

#endif
