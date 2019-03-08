//
// CLGLCD WinUSB interface
//
// Wrapper classes for WinUSB
//

#include <windows.h>
#include <setupapi.h>

#include "clglcd_exc.h"
#include "clglcd_winusb.h"

BOOL CLGLCD_Device::get_device_path(const GUID interface_GUID) {
  BOOL result = FALSE;
  HDEVINFO device_info;
  SP_DEVICE_INTERFACE_DATA interface_data;
  PSP_DEVICE_INTERFACE_DETAIL_DATA detail_data = NULL;
  ULONG length = 0;
  ULONG required_length = 0;
  //HRESULT hr;

  GUID GUID_local = interface_GUID;
  device_info = SetupDiGetClassDevs(&GUID_local, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  result = SetupDiEnumDeviceInterfaces(device_info, NULL, &GUID_local, 0, &interface_data);

  SetupDiGetDeviceInterfaceDetail(device_info, &interface_data, NULL, 0, &required_length, NULL);
  detail_data = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, required_length);
  if (detail_data == NULL) {
    SetupDiDestroyDeviceInfoList(device_info);
    return false;
  }

  detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  length = required_length;
  result = SetupDiGetDeviceInterfaceDetail(device_info, &interface_data, detail_data, length, &required_length, NULL);
  if (result == FALSE) {
    LocalFree(detail_data);
    SetupDiDestroyDeviceInfoList(device_info);
    return false;
  }

  strncpy(path, detail_data->DevicePath, sizeof(path)-1);

  SetupDiDestroyDeviceInfoList(device_info);
  LocalFree(detail_data);
  return result;
}


CLGLCD_Device::CLGLCD_Device(const GUID device_guid, ULONG timeout) {
  setup_handle = NULL;
  usb_handle = NULL;
  BOOL result = FALSE;

  memset(path, 0, sizeof(path));
  if (!get_device_path(device_guid))
    throw CLGLCD_device_error("Can't find connected CLGLCD USB device.");

  setup_handle = CreateFile(
    path,
    GENERIC_WRITE | GENERIC_READ,
    FILE_SHARE_WRITE | FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
    NULL
  );
  if (setup_handle == INVALID_HANDLE_VALUE) {
    DWORD err = GetLastError();
    if (err == ERROR_ACCESS_DENIED) throw CLGLCD_error("Access denied to USB device. Is device accessed by other process ?");
    throw CLGLCD_error("Error " + to_string(err) + "initializing device setup handle.");
  }

  if (!WinUsb_Initialize(setup_handle, &usb_handle))
    throw CLGLCD_error("Error initializing WinUSB handle.");

  int ep_count = 0;
  result = WinUsb_QueryInterfaceSettings(usb_handle, 0, &if_descriptor);
  if (result) {
    for(UCHAR i = 0; i < if_descriptor.bNumEndpoints; i++) {
      result = WinUsb_QueryPipe(usb_handle, 0, i, &pipe_info);
      if (result) {
        if ((pipe_info.PipeType == UsbdPipeTypeBulk) && USB_ENDPOINT_DIRECTION_OUT(pipe_info.PipeId)) {
          ep = pipe_info.PipeId;
          ep_count++;
        }
      }
    }
  }
  if (ep_count != 1) throw CLGLCD_device_error("Unrecognized CLGLCD output pipe configuration");
  if_num = if_descriptor.bInterfaceNumber;

  // Set pipe policy
  ULONG timeout_local = timeout;
  WinUsb_ResetPipe(usb_handle, ep);
  WinUsb_SetPipePolicy(usb_handle, ep, PIPE_TRANSFER_TIMEOUT, sizeof(timeout_local), &timeout_local);

  version = get_version();

  if (version.compare(0, 8, CLGLCD_MIN_VERSION) < 0) {
    throw CLGLCD_error("Unsupported CLGLCD firmware version");
  }
  calibration = get_calibration_report();
};

void CLGLCD_Device::cancel_IO() {
  if (!CancelIo(setup_handle)) {
    DWORD err = GetLastError();
    throw CLGLCD_device_error("Error " + to_string(err) + " canceling I/O on device");
  }
}

void CLGLCD_Device::reset_endpoint(int endpoint) {
  if (!WinUsb_ResetPipe(usb_handle, endpoint)) {
    DWORD err = GetLastError();
    throw CLGLCD_device_error("Error " + to_string(err) + " reseting endpoint " + to_string(endpoint));
  }
}

string CLGLCD_Device::get_version() {
  CHAR buf[64];
  memset(&buf, 0, sizeof(buf));
  WINUSB_SETUP_PACKET version_setup;
  memset(&version_setup, 0, sizeof(WINUSB_SETUP_PACKET));
  version_setup.Length = sizeof(buf)-1;
  version_setup.RequestType = 0xC0;
  version_setup.Index = if_num;
  version_setup.Request = CLGLCD_VERSION_REPORT;
  DWORD len = 0;
  if (!WinUsb_ControlTransfer(usb_handle, version_setup, (PUCHAR)buf, sizeof(buf)-1, &len, NULL)) {
    throw CLGLCD_device_error("WinUSB error " + to_string(GetLastError()) + " reading CLGLCD version");
  }
  return string(buf);
}

TP_CAL_REPORT* CLGLCD_Device::get_calibration_report() {
  TP_CAL_REPORT* calibration_data = NULL;
  calibration_data = (TP_CAL_REPORT*) LocalAlloc(LMEM_FIXED, sizeof(TP_CAL_REPORT));
  memset(calibration_data, 0, sizeof(TP_CAL_REPORT));
  WINUSB_SETUP_PACKET cal_setup;
  memset(&cal_setup, 0, sizeof(WINUSB_SETUP_PACKET));
  cal_setup.Length = sizeof(TP_CAL_REPORT)-1;
  cal_setup.RequestType = 0xC0;
  cal_setup.Index = if_num;
  cal_setup.Request = CLGLCD_TP_CAL_REPORT;
  DWORD recv_len = 0;
  if (!WinUsb_ControlTransfer(usb_handle, cal_setup, (PUCHAR)calibration_data, sizeof(TP_CAL_REPORT), &recv_len, NULL)) {
    throw CLGLCD_device_error("WinUSB error " + to_string(GetLastError()) + " reading CLGLCD touchpanel calbiration report");
  }
  if (recv_len < sizeof(TP_CALIBRATION)) {
    throw CLGLCD_device_error("Invalid CLGLCD touchpanel calibration report size" + to_string(recv_len));
  }
  return calibration_data;
}


CLGLCD_Transfer::CLGLCD_Transfer(CLGLCD_Device* device, size_t buffer_size) {
  dev = device;
  buffer = (uint8_t*) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, buffer_size);
  size = buffer_size;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  overlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
  completed = true;
}

CLGLCD_Transfer::~CLGLCD_Transfer() {
  if (buffer) {
    HeapFree(GetProcessHeap(), 0, buffer);
    buffer = NULL;
  }
  if (overlapped.hEvent) {
    CloseHandle(overlapped.hEvent);
  }
}

CLGLCD_Transfer* CLGLCD_Transfer::send_async() {
  if (completed) {
    throw CLGLCD_error("Re-use of completed data transfer, please reset it first");
  }
  if (!WinUsb_WritePipe(dev->usb_handle, dev->ep, buffer, size, NULL, &overlapped)) {
    DWORD err = GetLastError();
    if (err != ERROR_IO_PENDING) {
      throw CLGLCD_transfer_error("WinUSB error " + to_string(err) + " sending data transfer");
    }
  }
  return this;
}


CLGLCD_Transfer* CLGLCD_Transfer::wait(DWORD timeout) {
  if (!completed) {
    DWORD ovl_res = WaitForSingleObject(overlapped.hEvent, timeout);
    if (ovl_res != WAIT_OBJECT_0) {
      throw CLGLCD_transfer_error("Overlapped I/O wait object error " + to_string(ovl_res));
    }
  }
  return this;
}

CLGLCD_Transfer* CLGLCD_Transfer::reset() {
  completed = false;
  ResetEvent(overlapped.hEvent);
  return this;
}

size_t CLGLCD_Transfer::transferred_bytes() {
  if (completed) {
    throw CLGLCD_error("Re-use of completed control transfer, please reset it first");
  }
  DWORD transferred;
  if (!WinUsb_GetOverlappedResult(dev->usb_handle, &overlapped, &transferred, false)) {
    DWORD err = GetLastError();
    if (err != ERROR_IO_INCOMPLETE) {
      throw CLGLCD_transfer_error("WinUSB error " + to_string(err) + " reading input report");
    }
    return 0;
  }
  completed = true;
  return transferred;
}


CLGLCD_Control_Transfer::CLGLCD_Control_Transfer(CLGLCD_Device* device, size_t buffer_size) : CLGLCD_Transfer(device, buffer_size) {
  memset(&setup, 0, sizeof(WINUSB_SETUP_PACKET));
}

CLGLCD_Transfer* CLGLCD_Control_Transfer::send_async() {
  if (completed) {
    throw CLGLCD_error("Re-use of completed transfer, please reset it first");
  }
  if (!WinUsb_ControlTransfer(dev->usb_handle, setup, buffer, size, NULL, &overlapped)) {
    DWORD err = GetLastError();
    if (err != ERROR_IO_PENDING) {
      throw CLGLCD_transfer_error("WinUSB error " + to_string(err) + " sending control transfer");
    }
  }
  return this;
}

