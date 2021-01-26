#define LOG_TAG "M+_ServiceManagerHandler"

#include <mobileplus/ServiceManagerHandler.h>
#include <mobileplus/RemoteBinderManager.h>
#include <mobileplus/ConnectionManager.h>
#include <binder/IPCThreadState.h>
#include <mobileplus/Global.h>
#include <binder/TextOutput.h>

namespace android {

  ServiceManagerHandler* ServiceManagerHandler::serviceManager() {
    return this;
  }

  void ServiceManagerHandler::handleMessage(Message& message) {
    int32_t code = message.what;
    int32_t sock = message.arg1;
    uint32_t flags = message.arg2;
    Parcel *parcel = (Parcel*) message.obj;
    Parcel *reply = (flags & REPLY_OFF) ? NULL : new Parcel();
    ALOGI("handleMessage(), code = %d, parcel = 0x%p", code, parcel);

    switch (code) {
    case SharingType::REMOTE_GET_SERVICE_TRANSACTION: {
      getService(sock, *parcel, reply, flags);
      mConnectionManager->sendReply(sock, mHandle, *reply);
      break;
    }
    default: {
      break;
    }
    }
    parcel->freeDataNoInit();
    // parcel->clearObjects();
    // delete parcel;
    delete reply;
  }

  void ServiceManagerHandler::getService(int32_t sock, Parcel& data, Parcel* reply, uint32_t flags)
  {
    IPCThreadState::self()->transact(mHandle, GET_SERVICE_TRANSACTION, data, reply, flags);
    sp<IBinder> binderNode = reply->readStrongBinder();
    reply->setDataPosition(0);
    int32_t binderHandle = binderNode->remoteBinder()->handle();
    ALOGI("ServiceManagerHandler::getService() binderHandle %d", binderHandle);
    // Create a new binder node
    ServiceThread* serviceThread = mRemoteBinderManager->findServiceThread(binderHandle);
    if (serviceThread == NULL) {
      ALOGI("getService(), new ServiceThread(binderHandle = %d)", binderHandle);
      serviceThread = new ServiceThread(mRemoteBinderManager, mConnectionManager, binderNode, binderHandle, HandlerType::NORMAL_SERVICE_TYPE);
      mRemoteBinderManager->registerServiceThread(binderHandle, serviceThread);
      serviceThread->run("ServiceThread");
    }
  }

}; 
