
#include "clglcd.h"

void setup() {
  CLGLCD_init();  

  delay(2000); // Safety
  Serial.begin(9600);

  CLGLCD_on();
  CLGLCD_clear_screen();
}


void loop() {

  strcpy_P((char *)&screen[0][2], PSTR("320x240 Controllerless Graphics LCD"));
  strcpy_P((char *)&screen[1][11], PSTR("driven by Arduino"));
  strcpy_P((char *)&screen[14][12], PSTR("Fixed font test"));

  screen[2][0]   = 0xC9; // "╔"
  screen[2][39]  = 0xBB; // "╗"
  screen[13][0]  = 0xC8; // "╚"
  screen[13][39] = 0xBC; // "╝"
  for (uint8_t x=1; x<39; x++) {
    screen[2][x] = 0xCD; // "═"    
    screen[13][x] = 0xCD; // "═"
  }
  for (uint8_t y=3; y<13; y++) {
    screen[y][0] = 0xBA; // "║"    
    screen[y][39] = 0xBA; // "║"
  }

  uint8_t c = 0;
  uint8_t y = 3;
  while (true) {  
    for(uint8_t x=1; x<39; x++) {
      screen[y][x] = c++;
      delay(100);
    }
    y++;
    if (y > 12) {
      memcpy((char *)&screen[3], (char *)&screen[4], 10 * 40);
      y = 12;      
      screen[y][0] = 0xBA; // "║"    
      screen[y][39] = 0xBA; // "║"
      memset((void*)&screen[y][1], 0x20, 38);
    }    
  }  
}
