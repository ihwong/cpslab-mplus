#ifndef MOBILEPLUS_GLOBAL_H
#define MOBILEPLUS_GLOBAL_H

#include <utils/Log.h>
#include <binder/TextOutput.h>

#define TIME_OUT 1000
#define LISTEN_PORT 12130
#define SENSOR_PORT 12230

#define MAX_SOCK_NUM 30

#define REPLY_ON	0
#define REPLY_OFF	1

#define TYPE_0_MESSAGE 0
#define TYPE_1_MESSAGE 1
#define TYPE_2_MESSAGE 2
#define TYPE_3_MESSAGE 3
#define TYPE_4_MESSAGE 4

#define TYPE_0_MESSAGE_LEN 17
#define TYPE_1_MESSAGE_LEN 18
#define TYPE_2_MESSAGE_LEN 41
#define TYPE_3_MESSAGE_LEN 25

#define MAGIC_NUMBER 86178712

#define ONLY_PACKAGE_INFO 		0
#define APK_AND_PACKAGE_INFO 	1

#define UNUSED(x) (void)(x)
namespace android {

namespace SharingType{
	enum {
		REMOTE_SCHEDULE_SEND_RESULT_TRANSACTION = 997,
		REMOTE_GET_SERVICE_TRANSACTION = 999,
		
		REMOTE_START_ACTIVITY_TRANSACTION = 1000,
		REMOTE_BIND_SERVICE_TRANSACTION = 1001,
		REMOTE_BIND_ISOLATED_SERVICE_TRANSACTION = 1002,
		REMOTE_START_ACTIVITY_INTENT_SENDER_TRANSACTION = 1003,
		REMOTE_GET_CONTENT_PROVIDER_TRANSACTION = 1004,
			
	};

};

namespace HandlerType{
	enum {
		SERVICE_MANAGER_TYPE = 0,
		ACTIVITY_MANAGER_TYPE = 1,
		ACTIVITY_TASK_MANAGER_TYPE = 2,
		PACKAGE_MANAGER_TYPE = 3,
		NORMAL_SERVICE_TYPE = 4,
	};
};

namespace ParcelType{
	enum {
		LOCAL = 0,
		REMOTE = 1,
	};
};

namespace ActivityManager{
	enum {
		TRANSACTION_START_ACTIVITY = 7,
		TRANSACTION_GET_CONTENT_PROVIDER = 20,
		TRANSACTION_REF_CONTENT_PROVIDER = 22,
		TRANSACTION_START_SERVICE = 24,
		TRANSACTION_BIND_SERVICE = 26,
		TRANSACTION_BIND_ISOLATED_SERVICE = 27,
		TRANSACTION_PUBLISH_SERVICE = 30,
		TRANSACTION_REMOTE_BIND_ISOLATED_SERVICE = 198,
	};
};

namespace ActivityTaskManager{
	enum {
		TRANSACTION_START_ACTIVITY = 1,
		TRANSACTION_GET_ACTIVITY_CLASS_FOR_TOKEN = 108 ,
		TRANSACTION_START_VIRTUAL_ACTIVITY = 162,
		TRANSACTION_FINISH_VIRTUAL_ACTIVITY = 163,
		TRANSACTION_REMOTE_START_ACTIVITY = 164,
	};
};

namespace FdType {
	enum {
		UDS = 0,
		ION_MEMORY = 1,
		SYNC_FENCE = 2,
		ASH_MEMORY = 3,
	};
	
};

namespace PackagesManager{
	enum {
		//INITIATE_APP_REGISTERATION = ,
		GET_PACKAGE_INFO_TRANSACTION = 3,
		GET_PACKAGE_UID_TRANSACTION = 5,
		GET_PACKAGES_FOR_UID_TRANSACTION =37 , 
		QUERY_INTENT_ACTIVITIES_TRANSACTION = 48,
		QUERY_INTENT_SERVICES_TRANSACTION = 52,
		RESOLVE_CONTENT_PROVIDER_TRANSACTION= 58,
		//REMOTE_REGISTER_APP_TRANSACTION=,
		//QUERY_INTENT_PACKAGES_TRANSACTION= ,
		QUERY_INTENT_ACTIVITY_NAMES_TRANSACTION = 210,
		QUERY_INTENT_SERVICES_NAMES_TRANSACTION = 211,
	};
};

namespace ContentProvider {
	enum {
		OPEN_TYPED_ASSET_FILE_TRANSACTION = 23,
	};
};

namespace UriType{
	enum {
		STRING_URI =1,
		OPAQUE_URI =2,
		HIERARCHICAL_URI=3,		
	};
};

namespace FileType{
	enum {
		APK_FILE_TYPE = 0,
		PDF_FILE_TYPE = 1,
		IMAGE_FILE_TYPE = 2,
	};
};

};
#endif

