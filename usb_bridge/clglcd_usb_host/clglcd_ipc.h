//
// CLGLCD IPC interface
//
// Wrapper classes for Windows IPC
//


#ifndef CLGLCD_IPC_H
#define CLGLCD_IPC_H

#define CLGLCD_MUTEX_NAME "Global\\CLGLCD_MUTEX"
#define CLGLCD_SHM_NAME   "Local\\CLGLCD_SHM"

#include "Windows.h"

// Share memory structure
typedef struct _CLGLCD_SHM {
  // Written by usb_host
  char     version[8];
  uint32_t usb_host_ts;
  uint8_t  reserved1[52];

  // Touchscreen block, read from device
  uint32_t touch_ts;
  int32_t  touch_x;
  int32_t  touch_y;
  int32_t  touch_preasure;
  // Raw touchscreen data
  int32_t  raw_touch_x;
  int32_t  raw_touch_y;
  int32_t  raw_touch_z1;
  int32_t  raw_touch_z2;
  int32_t  raw_touch_cnt;
  uint8_t  reserved2[28];

  // Reserved
  uint8_t  reserved3[64];

  // Frame buffer selector - written by client(s)
  uint32_t active_ts;
  int32_t  active_buffer;
  uint8_t  reserved4[56];

  // Frame buffers - written by client(s)
  uint8_t  frame_buffer[2][240 * 320];

} CLGLCD_SHM;


class CLGLCD_ipc {
  public:
    CLGLCD_SHM* shm;
    bool client_is_connected();
    CLGLCD_ipc();
    ~CLGLCD_ipc();
  protected:
    HANDLE mutex;
    HANDLE file_map;
    HANDLE shm_handle;
};

#endif // CLGLCD_IPC_H
