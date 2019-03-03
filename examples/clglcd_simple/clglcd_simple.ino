// Simple CLGLCD test

// Display fixed bitmap in flash on
// Controllerless Graphics LCD module

#include "bitmap.h"

#define X_RES          320 // pixels
#define Y_RES          240 // pixels
#define REFRESH_RATE   70  // Hz

// Pin connection (Arduino Leonardo)
#define DISPOFF    D,6    // 12
#define DATA       B,7-4  // 11-8
#define ALT_M      E,6    // 7
#define CL2        D,7    // 6
#define CL1        C,6    // 5
#define FLM        D,4    // 4

// End of config

// 16MHz / 70Hz refresh / 240 lines ~= 952 ticks ~= 59.5us
#define CL1_TICKS  F_CPU / REFRESH_RATE / Y_RES

#define BIT_(p,b)     (b)
#define BIT(cfg)      BIT_(cfg)
#define PORT_(p,b)    (PORT ## p)
#define PORT(cfg)     PORT_(cfg)
#define PIN_(p,b)     (PIN ## p)
#define PIN(cfg)      PIN_(cfg)
#define DDR_(p,b)     (DDR ## p)
#define DDR(cfg)      DDR_(cfg)

#define SET(cfg)      PORT_(cfg) |= _BV(BIT_(cfg))
#define CLEAR(cfg)    PORT_(cfg) &= ~_BV(BIT_(cfg))
#define TOGGLE(cfg)   PIN_(cfg) = _BV(BIT_(cfg))
#define SWAP(b)       b = (b << 4) | (b >> 4)

void setup() {
  CLEAR(DISPOFF);
  DDR(DISPOFF) |= _BV(BIT(DISPOFF));
  delay(2000); // Safety

  // Another way to say pinMode(x, OUTPUT)
  DDR(DATA) = 0xF0;
  DDR(ALT_M) |= _BV(BIT(ALT_M));
  DDR(CL2) |= _BV(BIT(CL2));
  DDR(CL1) |= _BV(BIT(CL1));
  DDR(FLM) |= _BV(BIT(FLM));
  
  // Reset clocks and empty common drivers
  CLEAR(CL1);
  CLEAR(CL2);
  CLEAR(FLM);  
  for(uint8_t y = 0; y < Y_RES; y++) {
    TOGGLE(CL1);
    _NOP();
    TOGGLE(CL1);
  }

  // Setup Timer1 
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = CL1_TICKS - 1;
  // CTC mode 4, prescaller 1
  TCCR1B = (1<<WGM12) | (1<<CS10);
  // Clear OCF1A flag
  TIFR1  = (1 << OCF1A);

  // Enable display
  SET(DISPOFF);
}

void loop() {
  uint8_t x, y, bitmap_byte;
  uint16_t bitmap_ptr = (uint16_t)bitmap;

  // Set First Lime Marker
  SET(FLM);
  
  for(y = 0; y < Y_RES; y++) {    

    // Shift-in one row of bitmap
    // This takes about 45usec @ 16MHz
    for(x = 0; x < (X_RES / 8); x++) { 
      bitmap_byte = pgm_read_byte(bitmap_ptr++);
      TOGGLE(CL2); // up
      PORT(DATA) = bitmap_byte;
      TOGGLE(CL2); // down
      SWAP(bitmap_byte);
      TOGGLE(CL2); // up
      PORT(DATA) = bitmap_byte;
      TOGGLE(CL2); // down        
    }
    
    // Wait for timer period, i.e. OCF1A flag
    while ((TIFR1 & (1 << OCF1A)) == 0) {};

    // Latch the shifted row
    TOGGLE(CL1); // up
    _NOP(); // Minimum latch time is 100ns
    TOGGLE(CL1); // down
    
    // Clear FLM and toggle M on start of frame
    if (y == 0) {
      TOGGLE(ALT_M);
      CLEAR(FLM);
    }
    
    // Clear OCF1A flag
    TIFR1 = (1 << OCF1A);
  }      
}
