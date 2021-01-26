#define LOG_TAG "RemoteBinderManager.cpp"

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <mobileplus/BnRemoteBinderManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>

#include <mobileplus/ServiceThread.h>
#include <mobileplus/RemoteServiceManager.h>
#include <utils/Log.h>

#include <binder/TextOutput.h>

namespace android {

void RemoteBinderManager::instantiate(const char *ipAddr) {

	int32_t status;
    if (ipAddr) {
      status = defaultServiceManager()->addService(String16("android.app.IRemoteBinderManager"), new RemoteBinderManager(ipAddr));
    }

    else {
      status = defaultServiceManager()->addService(String16("android.app.IRemoteBinderManager"), new RemoteBinderManager());
    }
	
	ALOG(LOG_INFO, LOG_TAG, "NEW SERVICE REGISTERED!  with status %d", status );
}

RemoteBinderManager::RemoteBinderManager() {
    ALOG(LOG_INFO, LOG_TAG, "RemoteBinderManager SERVER is created");

	initBasicServices();
	initSharingMap();
	
    mConnectionManager.runSenderThread();
    mConnectionManager.runListenThread();
}

RemoteBinderManager::RemoteBinderManager(const char *ipAddr) {
    ALOG(LOG_INFO, LOG_TAG, "RemoteBinderManager CLIENT is created %s", ipAddr);
    
	initBasicServices();
	initSharingMap();
	
	mConnectionManager.runSenderThread();
    mConnectionManager.runListenThread();
    mConnectionManager.connectToServer(ipAddr);
}

RemoteBinderManager::~RemoteBinderManager() {
	
	close(mConnectionManager.mClientSock);
	for (int i=0; i<MAX_SOCK_NUM; i++){
		close(mConnectionManager.mListenThread->mSocks[i].fd);
	}

	close(mConnectionManager.mServerSock);


    ALOG(LOG_INFO, LOG_TAG, "RemoteBinderManager is destroyed");
}
  
status_t RemoteBinderManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
  
	Parcel request;
	request.appendFrom(&data, 0, data.dataSize());
	request.setDataPosition(0);
	String16 InterfaceName = request.getInterface();	
	
	bool match = false;
	status_t status = NO_ERROR;
	int32_t callingUid = IPCThreadState::self()->getCallingUid();
	String16 callingPackageName = getPackagesForUid(callingUid);
	string key = String8(callingPackageName).string();

	ALOG(LOG_INFO, LOG_TAG, "onTransact() callingPackage=%s, uid=%d, code=%d, flags=%d, data=%p, interfaceName=%s", String8(callingPackageName).string(), callingUid, code, flags, &data, String8(InterfaceName).string() );

	if(InterfaceName == String16("android.os.IServiceManager"))
	{
		//ALOG(LOG_INFO, LOG_TAG, "onTransact(), ServiceManager");
	  if (mSharingMap.find(key) != mSharingMap.end()) {
		      RemoteServiceManager* remoteSm = mRemoteProxyMap[mRemoteSmHandle]->serviceManager();
		      // RemoteActivityManager* remoteAM = mRemoteProxyMap[mRemoteAmHandle]->activityManager();
		      // request.enforceInterface(String16("android.os.IServiceManager"));
		      request.readString16();
		      request.readString16();
		      String16 serviceName = request.readString16();
		      request.print(alog);
		      request.setDataPosition(0);
		      int matchedIdx = matchWithSharingMap(&serviceName, callingPackageName, 999, -1);
		      if (matchedIdx != -1) {
			match = true;
			remoteSm->setReply(reply);
			remoteSm->getService(callingUid, callingPackageName, serviceName, request, reply, flags);
		      }

	  }

	  if (!match) {
	    reply->writeStrongBinder(nullptr);
	  }

	}
	
	else if(InterfaceName == String16("android.app.IActivityManager"))
	{
		if(mSharingMap.find(key) != mSharingMap.end()){
			
			RemoteActivityManager* remoteAM = mRemoteProxyMap[mRemoteAmHandle]->activityManager();

			switch(code){											 
				case (ActivityManager::TRANSACTION_BIND_ISOLATED_SERVICE):
				case (ActivityManager::TRANSACTION_BIND_SERVICE): {
					String16* results = nullptr;
					int resultSize = resolveService(request, &results);
					int matchedIdx = matchWithSharingMap(results, callingPackageName, SharingType::REMOTE_BIND_ISOLATED_SERVICE_TRANSACTION, resultSize);

					if(matchedIdx != -1){
						match = true;
						String16 calleePackageName = String16(results[matchedIdx]);
						remoteAM->setReply(reply);
						remoteAM->bindService(callingUid, callingPackageName, request, reply, flags);
					}
					delete[] results;													   
					break;
				}
			}
		}
	}
	else if (InterfaceName == String16("android.app.IActivityTaskManager"))
	{
		if(mSharingMap.find(key) != mSharingMap.end()){
			RemoteActivityTaskManager* remoteATM = mRemoteProxyMap[mRemoteATMHandle]->activityTaskManager();

			switch (code){
				case (ActivityTaskManager::TRANSACTION_START_ACTIVITY):
				{			
					String16* results = nullptr;
					int resultSize= resolveActivity(request, &results);					
					int matchedIdx = matchWithSharingMap(results, callingPackageName, SharingType::REMOTE_START_ACTIVITY_TRANSACTION, resultSize);
					
					if(matchedIdx != -1){
						match = true;
						String16 calleePackageName = String16(results[matchedIdx]);
						remoteATM->setReply(reply);
						remoteATM->startActivity(callingUid, callingPackageName, calleePackageName, request, reply, flags);
					}
			
					delete[] results;
					break;
				}
			}
		}
	}

	if (!match || status != NO_ERROR){
		ALOG(LOG_INFO, LOG_TAG, "no matching key in sharingMap ");
		reply->writeInt32(-1);
		return -1;
	}

	return BnRemoteBinderManager::onTransact(code, data, reply, flags);
}


void RemoteBinderManager::handleParcel(int32_t sock, const char* callerName, const char* filePath, int32_t handle, uint32_t code, uint32_t flags, Parcel *parcel)
{
	ServiceThread* sThread = findServiceThread(handle);
	if (sThread) {
		ServiceHandler* handler = sThread->getHandler();
		Message& msg = handler->obtainMessage(code);
		msg.arg1 = sock;
		msg.arg2 = flags;
		msg.obj = parcel;
		if (filePath)
			msg.obj2 = new String16(filePath);
		else
			msg.obj2 = NULL;

		ALOG(LOG_INFO, LOG_TAG, "handleParcel(), handle %d, code %d, parcel %p", handle, code, &parcel);
		handler->sendMessage(msg);
	}
}

void RemoteBinderManager::initBasicServices(){
	
	mConnectionManager.mRemoteBinderManager = this;
	mServiceManager	 = defaultServiceManager();
	mActivityManager = mServiceManager->checkService(String16("activity"));
	mActivityTaskManager = mServiceManager->checkService(String16("activity_task"));
	mPackageManager	 = mServiceManager->checkService(String16("package"));
	
	mSmHandle = 0;
	mAmHandle = ((BpBinder*) mActivityManager->remoteBinder())->handle();
	mATMHandle = ((BpBinder*) mActivityTaskManager->remoteBinder())->handle();
	mPmHandle = ((BpBinder*) mPackageManager->remoteBinder())->handle();

	ServiceThread* smThread = new ServiceThread(this, &mConnectionManager, NULL, mSmHandle, HandlerType::SERVICE_MANAGER_TYPE);
	ServiceThread* amThread = new ServiceThread(this, &mConnectionManager, mActivityManager, mAmHandle, HandlerType::ACTIVITY_MANAGER_TYPE);
	ServiceThread* atmThread= new ServiceThread(this, &mConnectionManager, mActivityTaskManager, mATMHandle, HandlerType::ACTIVITY_TASK_MANAGER_TYPE);
	ServiceThread* pmThread = new ServiceThread(this, &mConnectionManager, mPackageManager, mPmHandle, HandlerType::PACKAGE_MANAGER_TYPE);
	

	mServiceThreadMap.insert(pair<int, ServiceThread*>(mSmHandle, smThread));
	mServiceThreadMap.insert(pair<int, ServiceThread*>(mAmHandle, amThread));
	mServiceThreadMap.insert(pair<int, ServiceThread*>(mATMHandle, atmThread));
	mServiceThreadMap.insert(pair<int, ServiceThread*>(mPmHandle, pmThread));
	
	smThread->run("ServiceManagerThread");
	amThread->run("ActivityManagerThread");
	atmThread->run("ActivityTaskManagerThread");
	pmThread->run("PackageManagerThread"); 

  	ALOG(LOG_INFO, LOG_TAG, "initBasicServices(), SmHandle %d, AmHandle %d, ATMHandle %d, PmHandle %d", mSmHandle, mAmHandle, mATMHandle, mPmHandle);
}

// function about ServiceThread -> register, find, erase
void RemoteBinderManager::registerServiceThread(int handle, ServiceThread* serviceThread){
  	mServiceThreadMap.insert(pair<int, ServiceThread*>(handle, serviceThread));
}

ServiceThread* RemoteBinderManager::findServiceThread(int handle){
	unordered_map<int, ServiceThread*>::iterator itr = mServiceThreadMap.find(handle);
	return (itr == mServiceThreadMap.end()) ? nullptr : itr->second;
}

void RemoteBinderManager::eraseServiceThread(int handle){
  	unordered_map<int, ServiceThread*>::iterator itr = mServiceThreadMap.find(handle);
	if(itr != mServiceThreadMap.end())
			mServiceThreadMap.erase(itr);
}

void RemoteBinderManager::registerRemoteProxy(int handle, RemoteProxy* remoteProxy){
  	mRemoteProxyMap.insert(pair<int, RemoteProxy*>(handle, remoteProxy));
}

RemoteProxy* RemoteBinderManager::findRemoteProxy(int handle){
  	unordered_map<int, RemoteProxy*>::iterator itr = mRemoteProxyMap.find(handle);
	return (itr == mRemoteProxyMap.end()) ? nullptr : itr->second; 
}

void RemoteBinderManager::eraseRemoteProxy(int handle){
  	unordered_map<int, RemoteProxy*>::iterator itr = mRemoteProxyMap.find(handle);
	if(itr != mRemoteProxyMap.end())
		mRemoteProxyMap.erase(itr);
}

int32_t RemoteBinderManager::getBinderHandle(Parcel& parcel){
  	parcel.readInt32();
	parcel.readInt32();
	return parcel.readInt32();
}

String16 RemoteBinderManager::getPackagesForUid(int32_t uid)
{
	Parcel request, reply;
	request.writeInterfaceToken(String16("android.content.pm.IPackageManager"));
	request.writeInt32(uid);
	IPCThreadState::self()->transact(mPmHandle, PackagesManager::GET_PACKAGES_FOR_UID_TRANSACTION, request, &reply, 0);

	reply.readExceptionCode();
	reply.readInt32();
	String16 packageName = reply.readString16();
	return packageName;
}

void RemoteBinderManager::initSharingMap(){
  	{
		vector<SharingElem> * elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("com.example.toyapp_server"), SharingType::REMOTE_BIND_ISOLATED_SERVICE_TRANSACTION));
		elemVec->push_back(SharingElem(string("com.example.toyapp_server"), SharingType::REMOTE_START_ACTIVITY_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.example.toyapp_client"), elemVec));
	}

	{
		vector<SharingElem> * elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("com.facebook.katana"), SharingType::REMOTE_START_ACTIVITY_TRANSACTION));
		//elemVec->push_back(SharingElem(string("com.facebook.FacebookActivity"), SharingType::REMOTE_START_ACTIVITY_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.instagram.android"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("sensorservice"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.example.desk.accelercompare"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		// elemVec->push_back(SharingElem(string("com.android.vending"), SharingType::REMOTE_START_ACTIVITY_INTENT_SENDER_TRANSACTION));
		// elemVec->push_back(SharingElem(string("com.android.vending"), SharingType::REMOTE_BIND_SERVICE_TRANSACTION));
		elemVec->push_back(SharingElem(string("sensorservice"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.ea.games.r3_row"), elemVec));
	}

        {
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		// elemVec->push_back(SharingElem(string("com.android.vending"), SharingType::REMOTE_START_ACTIVITY_INTENT_SENDER_TRANSACTION));
		// elemVec->push_back(SharingElem(string("com.android.vending"), SharingType::REMOTE_BIND_SERVICE_TRANSACTION));
		elemVec->push_back(SharingElem(string("sensorservice"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("ru.andr7e.sensortest"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("sensorservice"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.donxu.lady_bug"), elemVec));
	}

        {
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("vibrator"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.example.myvib"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("vibrator"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.luxdelux.bodymassagervibration"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("clipboard"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.example.myclip"), elemVec));
	}

	{
		vector<SharingElem>* elemVec = new vector<SharingElem>();
		elemVec->push_back(SharingElem(string("clipboard"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.adobe.reader"), elemVec));
	}

	{
	        vector<SharingElem>* elemVec = new vector<SharingElem>();
	        elemVec->push_back(SharingElem(string("notification"), SharingType::REMOTE_GET_SERVICE_TRANSACTION));
		mSharingMap.insert(pair<string, vector<SharingElem>*>(string("com.example.mynoti"), elemVec));
	}
  
}
 
int RemoteBinderManager::matchWithSharingMap(String16* results, String16 callingPackageName, int code, int resultLen)
{
	if(results == nullptr)
			return -1;

	size_t resultSize = (resultLen != -1) ? resultLen : sizeof(*results)/sizeof(results[0]);

	string key = String8(callingPackageName).string();
	
	vector<SharingElem> *sharingVec = mSharingMap[key];
	vector<SharingElem>::iterator itr;

	for(int i=0; i<resultSize; i++){
		if(results[i] != callingPackageName){
			string target = String8(results[i]).string();

			for(itr = sharingVec->begin(); itr!=sharingVec->end(); itr++){

				if(target == itr->mPackageName && code == itr->mType){
					return i;
				}
			}
		}
	}

	return -1;
}

int RemoteBinderManager::resolveService(const Parcel &data, String16** results)
{
	const size_t* objects = (size_t *)data.objects();
	int objectsCount = data.objectsCount();
	size_t startPos = 0, len = 0;
	Parcel request, reply;
	
	ALOG(LOG_INFO, LOG_TAG, "resolveService()" );

	//enforceInterface()
	data.setDataPosition(0);
	data.readInt32();
	data.readInt32();
	data.readString16();
	//
	
	data.readStrongBinder(); 								//caller
	data.readStrongBinder(); 								//token
	
	startPos = data.dataPosition();
	len  = objects[objectsCount-1] - startPos;
	
	request.writeInterfaceToken(String16("android.content.pm.IPackageManager"));
	request.appendFrom(&data, startPos, len);
	request.writeInt32(0x00010000 | 0x00000400);
	request.writeInt32(0);

	
	IPCThreadState::self()->transact(mPmHandle, PackagesManager::QUERY_INTENT_SERVICES_NAMES_TRANSACTION, request, &reply, 0);
	
	reply.readExceptionCode();
	int listSize=reply.readInt32();
	if (listSize > 0){
		*results = new String16[listSize];
		for (int i=0; i<listSize; i++){
			String16 name = reply.readString16();
			(*results)[i] = name;
		}
	}

	data.setDataPosition(0);
	return listSize;
}


int RemoteBinderManager::resolveActivity(const Parcel &data, String16** results)
{
	const size_t* objects = (size_t *)data.objects();
	int objectsCount = data.objectsCount();
	size_t startPos = 0, len = 0;
	Parcel request, reply;

	ALOG(LOG_INFO, LOG_TAG, "resolveActivity() ");
	data.setDataPosition(0);
	data.readInt32();
	data.readInt32();
	if (data.readString16() != String16("android.app.IActivityTaskManager"))
		ALOG(LOG_INFO, LOG_TAG, "resolveActivity() wrong interfaceName" );
	
	data.readStrongBinder(); 								//caller
	String16 callingPackage = data.readString16(); 			//callingPackage;
	
	startPos = data.dataPosition();
	len  = objects[objectsCount-1] - startPos;

	request.writeInterfaceToken(String16("android.content.pm.IPackageManager"));
	request.appendFrom(&data, startPos, len);
	request.writeInt32(0x00010000 | 0x00000400);
	request.writeInt32(0);

	IPCThreadState::self()->transact(mPmHandle, PackagesManager::QUERY_INTENT_ACTIVITY_NAMES_TRANSACTION, request, &reply, 0);
	
	reply.readExceptionCode();
	int listSize = reply.readInt32();
	if (listSize > 0){
		*results = new String16[listSize];
		for (int i=0; i<listSize; i++){
			String16 name = reply.readString16();  // packageName or acitivityName
			(*results)[i] = name;
		}
	}

	data.setDataPosition(0);
	return listSize;
}


}; // end of namespace android
