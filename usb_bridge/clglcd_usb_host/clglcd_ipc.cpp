//
// CLGLCD IPC interface
//
// Wrapper classes for Windows IPC
//

#include "clglcd_exc.h"
#include "clglcd_ipc.h"

#include "versionhelpers.h"
#include "sddl.h"

//https://stackoverflow.com/questions/20580054/how-to-make-a-synchronization-mutex-with-access-by-every-process
// "D:(A;;GA;;;WD)(A;;GA;;;AN)S:(ML;;NW;;;S-1-16-0)
// "D:(A;;GA;;;WD)(A;;GA;;;AN)S:(ML;;NW;;;ME) -> for medium integrity
// "D:(A;;GA;;;WD)(A;;GA;;;AN)S:(ML;;NW;;;HI) -> for high integrity

PSECURITY_DESCRIPTOR MakeAllowAllSecurityDescriptor(void) {
  PSECURITY_DESCRIPTOR pSecDesc;
  if(IsWindowsVistaOrGreater()) {
    if(!ConvertStringSecurityDescriptorToSecurityDescriptor("D:(A;;GA;;;WD)(A;;GA;;;AN)S:(ML;;NW;;;ME)", SDDL_REVISION_1, &pSecDesc, NULL))
      throw CLGLCD_error("Error creating security descriptor");
  } else {
    if(!ConvertStringSecurityDescriptorToSecurityDescriptor("D:(A;;GA;;;WD)(A;;GA;;;AN)", SDDL_REVISION_1, &pSecDesc, NULL))
      throw CLGLCD_error("Error creating security descriptor");
  }
  return pSecDesc;
}


CLGLCD_ipc::CLGLCD_ipc() {
  PSECURITY_DESCRIPTOR pSecDesc = MakeAllowAllSecurityDescriptor();
  SECURITY_ATTRIBUTES SecAttr;
  SecAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  SecAttr.lpSecurityDescriptor = pSecDesc;
  SecAttr.bInheritHandle = FALSE;

  mutex = CreateMutex(&SecAttr, TRUE, CLGLCD_MUTEX_NAME);
  if (mutex == NULL) {
    throw CLGLCD_error("Error " + std::to_string(GetLastError()) + " creating global mutex");
  }

  //char shm_name[] = CLGLCD_SHM_NAME "\0";
  file_map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(CLGLCD_SHM), CLGLCD_SHM_NAME);
  if (file_map == NULL) {
      throw CLGLCD_error("Cannot allocate LCD shared memory");
  }
  shm = (CLGLCD_SHM*) MapViewOfFile(file_map, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CLGLCD_SHM));
  if (shm == NULL) {
    CloseHandle(file_map);
    file_map = NULL;
    throw CLGLCD_error("Error while memory mapping frame buffer");
  }
  memset(shm, 0, sizeof(CLGLCD_SHM));

  LocalFree(pSecDesc);
  ReleaseMutex(mutex);
}

CLGLCD_ipc::~CLGLCD_ipc() {
  if (file_map) {
    if (shm) UnmapViewOfFile(shm);
    CloseHandle(file_map);
  }
  if (mutex) {
    CloseHandle(mutex);
  }
};

bool CLGLCD_ipc::client_is_connected() {
  DWORD res = WaitForSingleObject(mutex, 0);
  if (res == WAIT_TIMEOUT) return true;
  if (res == WAIT_OBJECT_0 || res == WAIT_ABANDONED) {
    ReleaseMutex(mutex);
    return false;
  }
  if (res == WAIT_FAILED) {
    throw new CLGLCD_error("Error " + std::to_string(GetLastError()) + " while checking client mutex");
  }
  throw new CLGLCD_error("Unknown result " + std::to_string(res) + " while checking client mutex");
}
