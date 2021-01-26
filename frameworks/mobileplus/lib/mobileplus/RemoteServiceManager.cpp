#define LOG_TAG "M+_RemoteServiceManager.cpp"

#include <mobileplus/RemoteServiceManager.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>
#include <binder/TextOutput.h>

namespace android {

  status_t RemoteServiceManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
  {
    return NO_ERROR;
  }

  RemoteServiceManager* RemoteServiceManager::serviceManager() 
  {
    return this;
  }

  void RemoteServiceManager::getService(int32_t callingUid, String16 callerName, String16 serviceName, Parcel& data, Parcel* reply, uint32_t flags)
  {
    ALOGI("getService(), data = 0x%p, reply = 0x%p, flags = %d", &data, reply, flags);

    /*
    // Register the caller app into the server device
    hash_map<int, int>* remoteUidMap = &(mRemoteBinderManager->mRemoteUidMap);
    hash_map<int, int>::iterator itr = remoteUidMap->find(callingUid);
    int32_t remoteUid = -1;
    if (itr == remoteUidMap->end()) {
    int32_t pmHandle = mRemoteBinderManager->mRemotePmHandle;
    RemotePackageManager* remotePm = mRemoteBinderManager->mRemoteServiceMap[pmHandle]->packageManager();
    remoteUid = remotePm->remoteRegisterApp(callerName, ONLY_PACKAGE_INFO);
    remoteUidMap->insert(pair<int, int>(callingUid, remoteUid));
    }
    else 
    remoteUid = itr->second;

    // write additional informations
    data.writeRemoteUid(remoteUid);
    */
    // Send the parcel
    mConnectionManager->sendParcel(mSock, String8(callerName).string(), NULL, mHandle, 999/*SharingType::REMOTE_GET_SERVICE_TRANSACTION*/, flags, data);
    // Wait for the reply
    wait();

    // make a copy of reply parcel to avoid "Attempt to read from protected data in Parcel..."
    Parcel test;
    test.setData(reply->data(), reply->dataSize());
    // test.setObjects((size_t*)reply->objects(), reply->objectsCount());
    test.setDataPosition(0);
    
    // getBinderHandle from test (which is equivalent to reply, except for objects info)
    int32_t binderHandle = RemoteBinderManager::getBinderHandle(test);
    
    RemoteProxy* remoteProxy = mRemoteBinderManager->findRemoteProxy(binderHandle);
    if (remoteProxy == nullptr) {
      ALOGI("getService(), new remoteProxy (binderHandle = %d)", binderHandle);
      remoteProxy = new RemoteProxy(mRemoteBinderManager, mConnectionManager, mSock, binderHandle);
      mRemoteBinderManager->registerRemoteProxy(binderHandle, remoteProxy);
    }
    reply->clearObjects();
    reply->setDataPosition(0);
    // reply->freeDataNoInit();
    reply->writeStrongBinder(remoteProxy->mBinderNode);
    reply->setDataPosition(0);
    mReply = NULL;

  }

}; 
