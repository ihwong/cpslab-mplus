#define LOG_TAG "mobileplus_manager.cpp"

// header file location -------------------------------------
// ./frameworks/native/libs/binder/include/binder/IPCThreadState.h etc.
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
// -------------------------------------------------------------

#include <utils/Log.h>

#include <mobileplus/RemoteBinderManager.h>

using namespace android;

int main(int argc, char *argv[]) {

  ALOG(LOG_INFO, LOG_TAG, "MobilePlus is starting now...");

  // Case 1 : argument mismatch ----
  if (argc != 2) {
    ALOG(LOG_ERROR, LOG_TAG, "mplus requires argc to be 2");
    ALOG(LOG_ERROR, LOG_TAG, "$ mobileplus_manager [server / IP_ADDRESS]");
    return 0;
  }
  // -------------------------------

  // Case 2 : server side process ----
  if (strcmp(argv[1], "server") == 0) {
    ALOG(LOG_INFO, LOG_TAG, "Starting server process...");
    RemoteBinderManager::instantiate(NULL);
  }
  // ---------------------------------

  // Case 3 : client side process ----
  else {
    ALOG(LOG_INFO, LOG_TAG, "Starting client process...");
    RemoteBinderManager::instantiate(argv[1]);
  }
  // ---------------------------------
  
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
  
  return 0;
  
}
