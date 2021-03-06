
#include <Arduino.h>
#include "clglcd.h"

// Oranized for 64 8x16 'soft font' character in 16x4 screen buffer matrix
const unsigned char logo[CLGLCD_FONT_LINES][CLGLCD_SOFT_CHARS] __attribute__((progmem)) = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x7f, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xf0, 0x07, 0xff, 0xff, 0xc0, 0x0f, 0xff, 0xfe, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x00, 0xff, 0xff, 0x00, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x7f, 0xff, 0xc0, 0x00, 0x7f, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0x00, 0x03, 0xff, 0xfe, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x01, 0xff, 0xff, 0x80, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0xff, 0x00, 0x00, 0x1f, 0xff, 0xf8, 0x1f, 0xff, 0xfc, 0x00, 0x00, 0xff, 0xff, 0x00,
  0x07, 0xff, 0x80, 0x00, 0x00, 0x00, 0x01, 0xff, 0xff, 0x80, 0x00, 0x3f, 0x80, 0x01, 0xff, 0xe0,
  0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0xff, 0xfc, 0x00, 0x00, 0x0f, 0xff, 0xfc, 0x3f, 0xff, 0xf0, 0x00, 0x00, 0x3f, 0xff, 0x80,
  0x07, 0xff, 0x80, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x3f, 0x80, 0x01, 0xff, 0xe0,
  0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00,
  0x00, 0x00, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfc, 0x00, 0x00, 0x00,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0x03, 0xff, 0xfe, 0x7f, 0xff, 0xc0, 0x00, 0x00, 0x1f, 0xff, 0x80,
  0x07, 0xff, 0x80, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0x80, 0x01, 0xff, 0xe0,
  0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00,
  0x00, 0x00, 0x01, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0x80, 0x00, 0x00,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 0xff, 0x80,
  0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x3f, 0x80, 0x03, 0xff, 0xe0,
  0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00,
  0x00, 0x00, 0x07, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xe0, 0x00, 0x00,
  0x03, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f, 0x80, 0x0f, 0xff, 0xc0,
  0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x3f, 0x80, 0x03, 0xff, 0xe0,
  0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x00, 0x1f, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00,
  0x03, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x3f, 0x80, 0x07, 0xff, 0xc0,
  0x03, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x3f, 0x80, 0x07, 0xff, 0xc0,
  0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00,
  0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00,
  0x03, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xfc, 0x00, 0x3f, 0x80, 0x07, 0xff, 0xc0,
  0x03, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x07, 0xff, 0xc0,
  0x00, 0x00, 0x1f, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xf8, 0x00, 0x00,
  0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
  0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xf8, 0x00, 0x3f, 0x80, 0x03, 0xff, 0xe0,
  0x03, 0xff, 0xf0, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xc0,
  0x00, 0x00, 0x07, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0xff, 0xe0, 0x00, 0x00,
  0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00,
  0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xf0, 0x00, 0x3f, 0x80, 0x03, 0xff, 0xe0,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00, 0x1f, 0xff, 0x80,
  0x00, 0x00, 0x01, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xff, 0x80, 0x00, 0x00,
  0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x00,
  0x07, 0xff, 0x80, 0x00, 0x00, 0x00, 0x07, 0xff, 0xff, 0xe0, 0x00, 0x3f, 0x80, 0x01, 0xff, 0xe0,
  0x01, 0xff, 0xfc, 0x00, 0x00, 0x07, 0xff, 0xfe, 0x7f, 0xff, 0xe0, 0x00, 0x00, 0x3f, 0xff, 0x80,
  0x00, 0x00, 0x00, 0x1f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00,
  0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x03, 0xff, 0xff, 0xc0, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0xff, 0xfe, 0x00, 0x00, 0x0f, 0xff, 0xfc, 0x3f, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xff, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x01, 0xff, 0xff, 0x80, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x1f, 0xff, 0xfc, 0x00, 0x00, 0xff, 0xff, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x00, 0xff, 0xff, 0x00, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0x7f, 0xff, 0xc0, 0x00, 0xff, 0xff, 0xf0, 0x0f, 0xff, 0xff, 0x00, 0x03, 0xff, 0xfe, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00,
  0x07, 0xff, 0x80, 0x3f, 0xff, 0xe0, 0x00, 0x7f, 0xff, 0x00, 0x07, 0xff, 0xfc, 0x01, 0xff, 0xe0,
  0x00, 0x7f, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xe0, 0x07, 0xff, 0xff, 0xe0, 0x1f, 0xff, 0xfe, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  
};

void setup() {
  CLGLCD_init();  
  delay(2000); // Safety
  CLGLCD_on();
  CLGLCD_clear_screen();
  for(uint8_t y=0; y<CLGLCD_FONT_LINES; y++) {
    memcpy_P((void *)&soft_font[y], &logo[y], CLGLCD_SOFT_CHARS);
  }
}

void loop() {

  strcpy_P((char *)&screen[0][2], PSTR("320x240 Controllerless Graphics LCD"));
  strcpy_P((char *)&screen[1][11], PSTR("driven by Arduino"));
  strcpy_P((char *)&screen[14][12], PSTR("Soft font test"));

  uint8_t c = 256-CLGLCD_SOFT_CHARS;
  for(uint8_t y=6; y<10; y++) {
    for(uint8_t x=12; x<28; x++) {
      screen[y][x] = c++;
    }
  }

  while(true) {
    delay(50);
    for(uint8_t line=0; line<4; line++) {
      for(uint8_t y = 0; y < CLGLCD_FONT_LINES; y++) {        
        uint8_t x = line*16;              
        uint16_t shifted = (soft_font[y][x] << 1);
        uint8_t carry = (shifted >> 8);
        x += 15;
        // Why does GCC optimize away loops counting backwards ?
        for(uint8_t i=0; i<16; i++) {
          shifted = (soft_font[y][x] << 1);
          soft_font[y][x--] = (uint8_t)(shifted | carry);
          carry = (shifted >> 8);
        } 
      }
    }
  }  
  
}
