#ifndef MOBILEPLUS_ACTIVITYTASKMANAGERHANDLER_H
#define MOBILEPLUS_ACTIVITYTASKMANAGERHANDLER_H

#include <mobileplus/ServiceHandler.h>
#include <mobileplus/Global.h>

using namespace android::messaging;
using namespace std;

namespace android{

class ActivityTaskManagerHandler : public ServiceHandler{

public:
	ActivityTaskManagerHandler(RemoteBinderManager *remoteBinderManager, ConnectionManager *connectionManager, int32_t handle)
			: ServiceHandler(remoteBinderManager, connectionManager, handle){
		mInterface = String16("android.app.IActivityTaskManager");		
	}
	~ActivityTaskManagerHandler() {}
	virtual ActivityTaskManagerHandler* activityTaskManager();

	virtual void handleMessage(Message& message);

	void startActivity(Parcel& data, Parcel* reply, uint32_t flags, String16* filePath);
	void scheduleSendResult(Parcel& data, uint32_t flags);
	unordered_map<int, sp<IBinder>> mResultToMap;

private:
	String16 mInterface;

};

};

#endif
