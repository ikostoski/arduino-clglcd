#include <windows.h>
#include <time.h>
#include <cmath>

#include "clglcd_exc.h"
#include "clglcd_winusb.h"
#include "clglcd_ipc.h"

#ifdef STDOUT_DEBUG
#include <iostream>
#endif

using namespace std;

// Overlapping buffers
#define OVERLAPING_TRANSFERS  4        // Number of queued transfers
#define TRANSFER_WAIT         100      // miliseconds

// Values used for temporal dithering of LCD pixels
// Or in Arduino terminology: PWM for the pixels
static const uint16_t FRC_TABLE[16] = {
  0b0000000000000000, // 0
  0b0000000000000001, // 1
  0b0000000100000001, // 2
  0b0000010000100001, // 3
  0b0001000100010001, // 4
  0b0010010010010001, // 5
  0b0101001001010010, // 6
  0b0100101010010101, // 7
  0b0101010101010101, // 8
  0b0101011010101011, // 9
  0b0101101101011011, // 10
  0b0111011011011011, // 11
  0b0111011101110111, // 12
  0b0111111101111111, // 13
  0b1111111011111111, // 14
  0b1111111111111111, // 15
};

// The whole functions ignores the fact that we are probably
// running on 64bit CPU... This is just prototype so let
// the compiler make the best of it.
void render_frc_buffer(uint8_t *frame_data, uint8_t *usb_buffer, uint32_t rnd_seed, uint8_t frc_step) {
  uint8_t *frame_pos = frame_data;
  uint8_t *buf_pos = usb_buffer;
  uint32_t rnd32 = rnd_seed;

  for(uint8_t pckt_no=0; pckt_no<160; pckt_no++) {
    *buf_pos++ = pckt_no;
    // Dither and pack 480 frame bytes into 60 USB buffer bytes
    uint8_t d, p, b, z;
    for(uint8_t x=0; x<60; x++) {
      z = 0;
      for(uint8_t i=0; i<8; i++) {
        // xorshift32
        rnd32 ^= rnd32 << 13;
        rnd32 ^= rnd32 >> 17;
        rnd32 ^= rnd32 << 5;
        d = *frame_pos++;
        p = (d >> 4) & 0x0f;
        b = (FRC_TABLE[p] >> ((frc_step + rnd32) & 0x0f)) & 0x01;
        z = (z << 1) | b;
      }
      *buf_pos++ = z;
    }
    buf_pos += 3;
  }
};

// Convert raw touch-panel readings into screen coordinates
// based on calibration data reported by device
void calculate_touch_position(TP_STATE* touch, TP_CALIBRATION c9n, CLGLCD_SHM *lcd) {
  // copy raw data, if client wants to do its math
  lcd->raw_touch_x = touch->x;
  lcd->raw_touch_y = touch->y;
  lcd->raw_touch_z1 = touch->z1;
  lcd->raw_touch_z2 = touch->z2;
  lcd->raw_touch_cnt = touch->cnt;

  // Arduino signals invalid reading or reading in progress with z1 < 0
  // If reading is valid, process touch data with calibration
  if (touch->z1 >= 0) {
    int32_t x = max(0, min(319, (int)round((touch->x - c9n.x_min) * c9n.x_fact / 1024)));
    int32_t y = max(0, min(239, (int)round((touch->y - c9n.y_min) * c9n.y_fact / 1024)));
    int32_t z = max(0, min(100, (int)round(128 + (touch->z1 + x/8) - (touch->z2 + y/8))));
    if ((touch->z1 >= c9n.z1_min) && (touch->z2 <= c9n.z2_max)) {
      lcd->touch_x = x;
      lcd->touch_y = y;
      lcd->touch_preasure = z;
    } else {
      lcd->touch_x = -1;
      lcd->touch_y = -1;
      lcd->touch_preasure = 0;
    }
    lcd->touch_ts = touch->cnt;
  }
}


int main() {
  // Below lines are optional, raise process priority on Windows
  DWORD proc_pid = GetCurrentProcessId();
  HANDLE proc_handle = OpenProcess(PROCESS_ALL_ACCESS, true, proc_pid);
  SetPriorityClass(proc_handle, HIGH_PRIORITY_CLASS);

  // Represents CLGLCD bridge Arduino
  CLGLCD_Device* dev = new CLGLCD_Device(GUID_CLASS_CLGLCD, 100);
  #ifdef STDOUT_DEBUG
  cout << dev->version << " @ " << dev->path << endl;
  #endif

  // Interprocess mutex and shared memory API
  CLGLCD_ipc* ipc = new CLGLCD_ipc();
  CLGLCD_SHM *lcd = ipc->shm;
  strncpy(&lcd->version[0], dev->version.c_str(), 8);
  lcd->active_ts = 0;
  lcd->active_buffer = -1;

  try {

    // Setup frame transfers
    CLGLCD_Transfer* transfer[OVERLAPING_TRANSFERS];
    for (int i=0; i<OVERLAPING_TRANSFERS; i++) transfer[i] = new CLGLCD_Transfer(dev, TRANSFER_SIZE);

    uint8_t transfer_index = 0;
    uint8_t frc_step = 0;
    uint8_t *frame_buffer;
    bool active = false;

    // Start by queuing up control transfer for reading TP
    CLGLCD_Control_Transfer* tp_read = new CLGLCD_Control_Transfer(dev, sizeof(TP_REPORT));
    tp_read->reset();
    tp_read->setup.Length = sizeof(TP_REPORT);
    tp_read->setup.RequestType = 0xC0;
    tp_read->setup.Index = dev->if_num;
    tp_read->setup.Request = CLGLCD_TOUCH_REPORT;
    tp_read->send_async();

    while (true) { // Main loop

      //time_t ts = time(NULL);
      //time_t delta_ts = ts - lcd->active_ts;
      uint32_t ts = (uint32_t) time(NULL);
      uint32_t delta_ts = ts - lcd->active_ts;

      lcd->usb_host_ts = ts;

      if (active) {
        // Check which if any frame buffer is still active and if
        // client timestamp is recent (i.e. 60 seconds)
        if ((delta_ts > 60) || (lcd->active_buffer < 0) || (lcd->active_buffer > 1)) {
          // Wait out for any outstanding transfers
          for(int i=0; i<OVERLAPING_TRANSFERS; i++) {
            try { transfer[i]->wait(100); } catch (CLGLCD_transfer_error &e) {}
          }
          active = false;
          frc_step = 0;
          #ifdef STDOUT_DEBUG
          cout << "Display OFF, delta: " << delta_ts << endl;
          #endif // STDOUT_DEBUG
          Sleep(1);
          continue;
        }

        try {

          frame_buffer = lcd->frame_buffer[lcd->active_buffer];

          // Wait for queued transfer to finish
          transfer[transfer_index]->wait(TRANSFER_WAIT)->reset();

          // Prepare new buffer with FRC
          // TODO: Generate real random when frc_step is 0
          uint32_t rnd_seed = 0x1235678;
          render_frc_buffer(frame_buffer, transfer[transfer_index]->buffer, rnd_seed, frc_step);

          // And send it again...
          transfer[transfer_index]->send_async();

          if (++transfer_index >= OVERLAPING_TRANSFERS) transfer_index = 0;
          frc_step = (frc_step + 1) & 0x0f;

        } catch (CLGLCD_transfer_error &e) {
          dev->cancel_IO();
          #ifdef STDOUT_DEBUG
          cout << '.';
          #endif
          for (int i = 0; i < OVERLAPING_TRANSFERS; i++)
            transfer[i]->wait(10);
          dev->reset_endpoint(dev->ep);
        }


      } else { // not active
        // Check if client activated the display

        if ((lcd->active_buffer > 0) && (lcd->active_buffer < 2) && (delta_ts < 60)) {
          #ifdef STDOUT_DEBUG
          cout << "Display ON, frame buffer " << lcd->active_buffer << ", delta:" << delta_ts << endl;
          #endif // STDOUT_DEBUG
          dev->reset_endpoint(dev->ep);
          active = true;
          continue;
        }

        tp_read->wait(TRANSFER_WAIT);
        Sleep(10);
      }

      // Check touch-panel read result
      int read_bytes;
      try {
        read_bytes = tp_read->transferred_bytes();
      } catch (CLGLCD_transfer_error &e) {
        // If we cant read touch-panel, likely device disconnected (or reset)
        // Check for ERROR_GEN_FAILURE (31)
        throw CLGLCD_device_error(e.what());
      }


      if (read_bytes == sizeof(TP_STATE)) {
        TP_STATE* touch = (TP_STATE*) tp_read->buffer;
        calculate_touch_position(touch, dev->calibration->data, lcd);

        // Restart control transfer
        tp_read->reset();
        tp_read->send_async();
      }


    } // end while (true)

  } catch (CLGLCD_error &e) {
    #ifdef STDOUT_DEBUG
    cerr << e.what() << endl;
    #endif
    lcd->usb_host_ts = 0;
    return 1;
  }

  return 0;
}






