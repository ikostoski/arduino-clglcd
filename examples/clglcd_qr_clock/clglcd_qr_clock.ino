//
// QR clock on Controllerless LCD. 
//
// Uses I2C DS1307 RTC module and RTCLib library
// Uses Adafruit (4-wire resistive) TouchScreen library
//
// Original idea and sources used
// http://ch00ftech.com/2013/04/23/optimizing-the-qr-clock/
// https://github.com/arduinoenigma/EnigmaQRClock
//

#include <Wire.h>
#include "RTClib.h"
#include "TouchScreen.h"

#include "clglcd_config.h"
#include "clglcd_font.h"
#include "clglcd.h"

// Touch panel config 
#define TP_XL       A0
#define TP_YU       A1
#define TP_XR       A2
#define TP_YD       A3
#define TP_MIN_Z    200

// Touch buttons position and size
#define TP_VB_AREA  50
#define TP_SWITCH_X 735
#define TP_SWITCH_Y 275
#define TP_INVERT_X 270
#define TP_INVERT_Y 275
#define TP_ROTATE_X 270
#define TP_ROTATE_Y 735



// Based on the font used
#define WHITE  0x00
#define BLACK  0x0F

// Adjust defaults if needed
uint8_t enable = 1;
uint8_t invert = 1;
uint8_t rotate = 0;

TouchScreen ts = TouchScreen(TP_XL, TP_YU, TP_XR, TP_YD, 0);
bool touch_sense = true;

RTC_DS1307 rtc;
unsigned char last_second;
char adjust_buffer[32];
uint8_t adjust_cnt = 0;

char display_string[18];
unsigned char outputmatrix[56];

// Defined in "choof_qr_code.c"
extern "C" {
  unsigned char getbit(unsigned char * array, int pointer);
  void generate_qr_code (const char *input, unsigned char *outputmatrix);
}

void clear_screen() {
  if (invert) {
    memset((void*)&screen, WHITE, sizeof(screen));
  } else {
    memset((void*)&screen, BLACK, sizeof(screen));
  }
}

// Draws 21x21 QR code using characters 0-15 setup to 
// represent bitmap of four quadrants of character 
// box (8x10). For the X axis, QR bits are 3:2 expanded 
// (i.e. each QR bit is drawn with 1.5 charactes, or 3 quadrants). 
// Since QR code has uneven number of points, the Y offset 
// on 24 character lines was noticable, and everything is drawn 
// shifted down by half character.
void draw_matrix(unsigned char *matrix) {
  unsigned char x, y;  
  unsigned char c1, c2;
  unsigned char *scr_pos;
  unsigned char prev_row[11], c1p, c2p;
  unsigned char inv_mask = (1-invert);
  
  memset(&prev_row, 3*inv_mask, 10);
  prev_row[10] = inv_mask;
  
  for (y=0; y<21; y++) {
    scr_pos = (unsigned char*)&screen[y+1][4];
    for (x = 0; x < 10; x++) {
      c1p = prev_row[x] >> 1;
      c2p = prev_row[x] & 1;
      if (rotate) {
        c1 = getbit(matrix, 42*x + y) ^ invert;
        c2 = getbit(matrix, 42*x + 21 + y) ^ invert;
      } else {
        c1 = getbit(matrix, 21*y + 2*x) ^ invert;
        c2 = getbit(matrix, 21*y + 2*x + 1) ^ invert;
      }
      *scr_pos++ = 12*c1p + 3*c1;
      *scr_pos++ = 8*c1p + 4*c2p + 2*c1 + c2;
      *scr_pos++ = 12*c2p + 3*c2;
      prev_row[x] = 2*c1 + c2;
    }

    c1p = prev_row[10];
    if (rotate) {
      c1 = getbit(matrix, 21*20 + y) ^ invert;
    } else {
      c1 = getbit(matrix, 21*y + 20) ^ invert;
    }
    *scr_pos++ = 12*c1p + 3*c1;
    *scr_pos = 8*c1p + 2*c1 + 5*inv_mask;
    prev_row[10] = c1;
  }

  // Last half-row
  scr_pos = (unsigned char*)&screen[22][4];
  for (x = 0; x < 10; x++) {
    c1p = prev_row[x] >> 1;
    c2p = prev_row[x] & 1;
    *scr_pos++ = 12*c1p + 3*inv_mask;
    *scr_pos++ = 8*c1p + 4*c2p + 3*inv_mask;
    *scr_pos++ = 12*c2p + 3*inv_mask;
  }
  c1p = prev_row[10];
  *scr_pos++ = 12*c1p + 3*inv_mask;
  *scr_pos = 8*c1p + 7*inv_mask;  
}

void setup() {
  CLGLCD_init();  

  delay(1000); 
  Serial.begin(9600);
  clear_screen();  
  if (enable) CLGLCD_on();

  memset(display_string, 0, sizeof(display_string));
  
  if (!rtc.begin()) {
    sprintf(display_string, "RTC INIT ERR;");
    generate_qr_code(display_string, outputmatrix);
    draw_matrix(outputmatrix);
    while (true);
  }    
}


void loop() {
  DateTime dt;

  if (rtc.isrunning()) {
    
    do {
      delay(50);

      // Check for touch
      TSPoint p = ts.getPoint();
      if (p.z > 200) {
        if (touch_sense) {
          if ((abs(TP_INVERT_X - p.x) < TP_VB_AREA) && (abs(TP_INVERT_Y - p.y) < TP_VB_AREA)) {
            invert ^= 1;
            clear_screen();
            draw_matrix(outputmatrix);
          } 
          if ((abs(TP_ROTATE_X - p.x) < TP_VB_AREA) && (abs(TP_ROTATE_Y - p.y) < TP_VB_AREA)) {
            rotate ^= 1;
            clear_screen();
            draw_matrix(outputmatrix);
          }
          if ((abs(TP_SWITCH_X - p.x) < TP_VB_AREA) && (abs(TP_SWITCH_Y - p.y) < TP_VB_AREA)) {
            enable ^= 1;
            if (enable) {
              CLGLCD_on();
            } else {
              CLGLCD_off();
            }
          }
          touch_sense = false;
        } 
      } else {
        touch_sense = true;
      }      

      // Monitor Serial for time adjust string
      // sample input: date = "Dec-26-2009", time = "12:34:56", see RTCLib      
      while (Serial.available()) {
        char c = Serial.read();
        if (adjust_cnt < (sizeof(adjust_buffer) - 1)) {
          adjust_buffer[adjust_cnt++] = c;
        }
        if (c == '\r') {
          adjust_buffer[adjust_cnt] = '\0';
          adjust_cnt = 0;                   
          char *sdate = strtok(adjust_buffer, " ");
          char *stime = strtok(NULL, "\r\n\0");
          if ((sdate != NULL) && (stime != NULL)) {
            Serial.print("Set "); Serial.print(sdate); 
            Serial.print(" "); Serial.println(stime);    
            rtc.adjust(DateTime(sdate, stime));
          }          
        }
      }       

      // Read RTC
      dt = rtc.now();      
    } while (dt.second() == last_second);
    last_second = dt.second();
  
    sprintf(display_string, "T %02d:%02d:%02d;", dt.hour(), dt.minute(), dt.second()); 
    generate_qr_code(display_string, outputmatrix);
    draw_matrix(outputmatrix);
    
  } else {
  
    sprintf(display_string, "RTC STOPPED;");
    generate_qr_code(display_string, outputmatrix);
    draw_matrix(outputmatrix);
    delay(1000);
      
  }

}
