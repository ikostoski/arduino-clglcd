//
// CLGLCD WinUSB interface
//
// Wrapper classes for WinUSB
//

#ifndef CLGLCD_WINUSB_H
#define CLGLCD_WINUSB_H

#include <windows.h>
#include <winusb.h>

// DeviceClassGUID for the controller
DEFINE_GUID(GUID_CLASS_CLGLCD, 0x1DC9A650L, 0x260B, 0x4128, 0x9F, 0x0E, 0x43, 0x4C, 0x47, 0x4C, 0x43, 0x44);

// Version
#define CLGLCD_MIN_VERSION     "CLGLCD10"

// Reports
#define CLGLCD_VERSION_REPORT  0
#define CLGLCD_TP_CAL_REPORT   1
#define CLGLCD_TOUCH_REPORT    2

#define TRANSFER_SIZE          160 * 64 // 160 packet * (60 bytes + overhead)
#define MAX_DEVPATH_LENGTH     256

using namespace std;

#pragma pack(1)
typedef struct _TP_STATE {
  int8_t z1;
  int8_t z2;
  int16_t x;
  int16_t y;
  uint16_t cnt;
} TP_STATE;
#pragma pack()

#pragma pack(1)
typedef struct _TP_REPORT {
  TP_STATE state;
   uint8_t reserved[54];
} TP_REPORT;
#pragma pack()

#pragma pack(1)
typedef struct _TP_CALIBRATION {
  int16_t x_min;
  int16_t x_fact;
  int16_t y_min;
  int16_t y_fact;
  int16_t z1_min;
  int16_t z2_max;
} TP_CALIBRATION;
#pragma pack()

#pragma pack(1)
typedef struct _TP_CAL_REPORT {
  TP_CALIBRATION data;
  uint8_t reserved[50];
} TP_CAL_REPORT;
#pragma pack()

// class has pointer data members but does not override
class CLGLCD_Device {
  public:
    HANDLE usb_handle;
    CHAR path[MAX_DEVPATH_LENGTH];
    BYTE if_num;
    BYTE ep;
    string version;
    TP_CAL_REPORT* calibration;
    CLGLCD_Device(const GUID InterfaceGuid, ULONG timeout);
    // TODO variant with serial number
    ~CLGLCD_Device();
    void cancel_IO();
    void reset_endpoint(int endpoint);
  protected:
    HANDLE setup_handle;
    USB_INTERFACE_DESCRIPTOR if_descriptor;
    WINUSB_PIPE_INFORMATION pipe_info;
    BOOL get_device_path(const GUID interface_GUID);
    string get_version();
    TP_CAL_REPORT* get_calibration_report();
};

//class  has virtual functions and accessible non-virtual destructor

class CLGLCD_Transfer {
  public:
    WINUSB_SETUP_PACKET setup;
    uint8_t *buffer;
    size_t size;
    size_t result_size;
    OVERLAPPED overlapped;
    bool completed;
    CLGLCD_Transfer(CLGLCD_Device* device, size_t size);
    virtual ~CLGLCD_Transfer();
    virtual CLGLCD_Transfer* send_async();
    virtual CLGLCD_Transfer* wait(DWORD timeout);
    virtual CLGLCD_Transfer* reset();
    virtual size_t transferred_bytes();
  protected:
    CLGLCD_Device* dev;
};

class CLGLCD_Control_Transfer: public CLGLCD_Transfer {
  public:
    WINUSB_SETUP_PACKET setup;
    CLGLCD_Control_Transfer(CLGLCD_Device* device, size_t buffer_size);// : CLGLCD_Transfer(device, buffer_size);
    virtual CLGLCD_Transfer* send_async() override;
};


#endif // CLGLCD_WINUSB_H
