/*
 * PluggableUSB/WCID Implementation
 *
 * Copyright (c) 2019 Ivan Kostoski
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */

// For WCID, see: https://github.com/pbatard/libwdi/wiki/WCID-Devices


#include "clglcd_config.h"
#include "clglcd_usb_wcid.h"

#define CLGLCD_USB_VERSION       "CLGLCD10"
#define CLGLCD_USB_SERIAL        "CLGLCD"
#define CLGLCD_USB_EP_SIZE       64
#define CLGLCD_USB_PROTOCOL      0xFF
#define CLGLCD_USB_OUT_INTERVAL  0x80
#define CLGLCD_USB_INTERFACE_NUM 0x02

#define D_WCID_OS_DESRIPTOR_ID    0xEE
#define D_WCID_VENDOR_CODE        0xC1
#define D_WCID_MSFT100            0x4D, 0x00, 0x53, 0x00, 0x46, 0x00, 0x54, 0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00

const uint8_t WCID_MS_OS_DESRIPTOR[] PROGMEM = {0x12, 0x03, D_WCID_MSFT100, D_WCID_VENDOR_CODE, 0x00}; 

#define D_WCID_REQUEST_TYPE       (REQUEST_VENDOR | REQUEST_DEVICETOHOST)
#define D_WCID_FEATURE_DESC_ID    0x04
#define D_WCID_BCOUNT             0x02
#define D_WCID_FD_LEN             0x40
#define D_WCID_BCD_VERSION        0x00, 0x01
#define D_WCID_RSV7               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define D_WCID_WINUSB             0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00
#define D_WCID_RSV8               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define D_WCID_RSV6               0x00, 0x00, 0x00, 0x00, 0x00, 0x00

const uint8_t WCID_FEATURE_DESCRIPTOR[] PROGMEM = {
  // Header 16-bytes
  D_WCID_FD_LEN, 0x00, 0x00, 0x00, 
  D_WCID_BCD_VERSION, 
  D_WCID_FEATURE_DESC_ID, 0x00, 
  D_WCID_BCOUNT,
  D_WCID_RSV7,
  // CDC Feature
  0x00, 0x01, D_WCID_RSV8, D_WCID_RSV8, D_WCID_RSV6, 
  // CLGLCD Feature
  CLGLCD_USB_INTERFACE_NUM, 0x01, D_WCID_WINUSB, D_WCID_RSV8, D_WCID_RSV6 
};

#define D_WCID_EPFD_LEN      0x8e 
#define D_WCID_EPFD_ID       0x05
#define D_WCID_WCOUNT        0x01
#define D_WCID_EPFD_PROP_LEN 0x84, 0x00, 0x00, 0x00
#define D_WCID_PROP_REG_SZ   0x01, 0x00, 0x00, 0x00
#define D_WCID_EPFD_NAME_LEN 0x28, 0x00
#define D_WCID_DI_GUID_NAME /* UNICODE "DeviceInterfaceGUID\0" */\
  0x44, 0x00, 0x65, 0x00, 0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x49, 0x00, 0x6e, 0x00, \
  0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00, 0x47, 0x00, \
  0x55, 0x00, 0x49, 0x00, 0x44, 0x00, 0x00, 0x00
#define D_WCID_EPFD_DATA_LEN 0x4e, 0x00, 0x00, 0x00
#define D_WCID_DI_GUID_DATA /* UNICODE "{1DC9A650-260B-4128-9F0E-434C474C4344}\0" */ \
  0x7B, 0x00, 0x31, 0x00, 0x44, 0x00, 0x43, 0x00, 0x39, 0x00, 0x41, 0x00, 0x36, 0x00, 0x35, 0x00, \
  0x30, 0x00, 0x2D, 0x00, 0x32, 0x00, 0x36, 0x00, 0x30, 0x00, 0x42, 0x00, 0x2D, 0x00, 0x34, 0x00, \
  0x31, 0x00, 0x32, 0x00, 0x38, 0x00, 0x2D, 0x00, 0x39, 0x00, 0x46, 0x00, 0x30, 0x00, 0x45, 0x00, \
  0x2D, 0x00, 0x34, 0x00, 0x33, 0x00, 0x34, 0x00, 0x43, 0x00, 0x34, 0x00, 0x37, 0x00, 0x34, 0x00, \
  0x43, 0x00, 0x34, 0x00, 0x33, 0x00, 0x34, 0x00, 0x34, 0x00, 0x7D, 0x00, 0x00, 0x00
  
const uint8_t WCID_EPFD[] PROGMEM = {
  // Header - 10 bytes is sent of first query
  D_WCID_EPFD_LEN, 0x00, 0x00, 0x00,
  D_WCID_BCD_VERSION, 
  D_WCID_EPFD_ID, 0x00, 
  D_WCID_WCOUNT, 0x00,
  // Section 1 - requested later
  D_WCID_EPFD_PROP_LEN, 
  D_WCID_PROP_REG_SZ, 
  D_WCID_EPFD_NAME_LEN, D_WCID_DI_GUID_NAME,
  D_WCID_EPFD_DATA_LEN, D_WCID_DI_GUID_DATA,
};

const uint8_t TP_CAL_REPORT[] PROGMEM = {
  lowByte(CLGLCD_TP_CAL_XM), highByte(CLGLCD_TP_CAL_XM),
  lowByte(CLGLCD_TP_CAL_XF), highByte(CLGLCD_TP_CAL_XF),
  lowByte(CLGLCD_TP_CAL_YM), highByte(CLGLCD_TP_CAL_YM),
  lowByte(CLGLCD_TP_CAL_YF), highByte(CLGLCD_TP_CAL_YF),
  lowByte(CLGLCD_TP_CAL_Z1_MIN), highByte(CLGLCD_TP_CAL_Z1_MIN),
  lowByte(CLGLCD_TP_CAL_Z2_MAX), highByte(CLGLCD_TP_CAL_Z2_MAX),  
};
  
typedef struct {
  InterfaceDescriptor     desc1;
  EndpointDescriptor      out1;
} VendorDescriptor;

CLGLCD_USB_::CLGLCD_USB_(void): PluggableUSBModule(1, 1, epType) {
  epType[0] = EP_TYPE_BULK_OUT;  
  tp_report = {0xFF, 0, 0, 0, 0, 0, 0, 0}; // .z1=-1, .z2=0, .x=0, .y=0, .cnt=0};
  PluggableUSB().plug(this);
}

int CLGLCD_USB_::getInterface(uint8_t* interfaceCount) {
  *interfaceCount += 1;
  VendorDescriptor _vendorDescriptor = {
    D_INTERFACE(pluggedInterface, 1, CLGLCD_USB_PROTOCOL, 0, 0),
    D_ENDPOINT(USB_ENDPOINT_OUT(pluggedEndpoint), USB_ENDPOINT_TYPE_BULK, CLGLCD_USB_EP_SIZE, CLGLCD_USB_OUT_INTERVAL),
  };
  return USB_SendControl(0, &_vendorDescriptor, sizeof(_vendorDescriptor));
}

int CLGLCD_USB_::getDescriptor(USBSetup& setup __attribute__((unused))) {  
  // Send 'Microsoft OS String Descriptor' at offset 0xEE
  if ((setup.wValueH == USB_STRING_DESCRIPTOR_TYPE) && (setup.wValueL == D_WCID_OS_DESRIPTOR_ID)) {
    return USB_SendControl(TRANSFER_PGM, WCID_MS_OS_DESRIPTOR, sizeof(WCID_MS_OS_DESRIPTOR));
  }
  return 0;
}

bool CLGLCD_USB_::setup(USBSetup& setup) { 
  // USB_Core will pass 'Class' or 'Vendor' requests to PluggableUSB::setup chain

  // WCID reqests handler, global for device
  if (((setup.bmRequestType & D_WCID_REQUEST_TYPE) == D_WCID_REQUEST_TYPE) && (setup.bRequest == D_WCID_VENDOR_CODE)) {
    if (setup.wIndex == D_WCID_FEATURE_DESC_ID) {
      int len = min(setup.wLength, sizeof(WCID_FEATURE_DESCRIPTOR));
      USB_SendControl(TRANSFER_PGM, WCID_FEATURE_DESCRIPTOR, len);
      return true;
    } else if ((setup.wIndex == D_WCID_EPFD_ID) && (setup.wValueL == pluggedInterface)) {
      int len = min(setup.wLength, D_WCID_EPFD_LEN);
      USB_SendControl(TRANSFER_PGM, WCID_EPFD, len);
      return true;
    } else {
      return false;
    }
  }
    
  // Only for this interface 
  if (setup.wIndex != pluggedInterface) return false;
  
  // GET requests
  if (setup.bmRequestType & REQUEST_DEVICETOHOST) {    
    if (setup.bRequest == 0) {
      USB_SendControl(TRANSFER_PGM, F(CLGLCD_USB_VERSION), sizeof(CLGLCD_USB_VERSION));
      return true;
    } else if (setup.bRequest == 1) {         
      USB_SendControl(TRANSFER_PGM, TP_CAL_REPORT, sizeof(TP_CAL_REPORT));
      return true;
    } else if (setup.bRequest == 2) {         
      return USB_SendControl(0, tp_report.buf, sizeof(tp_report));
      return true;
    }
  }
  return false;
}

uint8_t CLGLCD_USB_::getShortName(char *name) {
  // This is actually our serial number
  memcpy_P(name, F(CLGLCD_USB_SERIAL), sizeof(CLGLCD_USB_SERIAL));
  return sizeof(CLGLCD_USB_SERIAL);
}

uint8_t CLGLCD_USB_::getEP() {
  return pluggedEndpoint;
};
