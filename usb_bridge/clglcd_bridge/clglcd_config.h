//
// Hardware configuration
//

#ifndef CLGLCD_CONFIG
#define CLGLCD_CONFIG

//
//   F-51543NFU  <>  Arduino
//
//   1  V LED-   => (GND trough backlight MOSFET)
//   2  V LED+   <= (Power, +5V)
//   3  DISPOFF  <= (Needs pull-down to GND)
//   4  D3       <= (x,7)
//   5  D2       <= (x,6)
//   6  D1       <= (x,5)
//   7  D0       <= (x,4)
//   8  VEE      => (Power, -24V)
//   9  VSS      == (GND)
//   10 VDD      <= (Power, +5V)
//   11 V0       => (Power, -16.8V from trimmer)
//   12 M        <= (PWM, same timer as CL1)
//   13 CL2      <= (PWM)
//   14 CL1      <= (PWM, same timer as M)
//   15 FLM      <= (any)

#if defined(ARDUINO_AVR_LEONARDO)

#define CLGLCD_BCKL_CTRL   D,0
#define CLGLCD_DISPOFF     D,6
#define CLGLCD_DATA        B,4-7
#define CLGLCD_VEE_CTRL    E,6
#define CLGLCD_ALT_M       D,7
#define CLGLCD_ALT_M_OCnX  4,D
#define CLGLCD_CL2         C,6
#define CLGLCD_CL2_OCnX    3,A
#define CLGLCD_CL1         C,7
#define CLGLCD_CL1_OCnX    4,A
#define CLGLCD_FLM         D,4

#define CLGLCD_TP_XL_AN    A0
#define CLGLCD_TP_XL       F,7
#define CLGLCD_TP_YU       F,6
#define CLGLCD_TP_XR       F,5
#define CLGLCD_TP_YL_AN    A3
#define CLGLCD_TP_YL       F,4

#elif defined(ARDUINO_AVR_PROMICRO)

#define CLGLCD_BCKL_CTRL   D,3
#define CLGLCD_DISPOFF     D,2
#define CLGLCD_DATA        F,4-7
#define CLGLCD_VEE_CTRL    D,0
#define CLGLCD_ALT_M       B,5
#define CLGLCD_ALT_M_OCnX  1,A
#define CLGLCD_CL2         C,6
#define CLGLCD_CL2_OCnX    3,A
#define CLGLCD_CL1         B,6
#define CLGLCD_CL1_OCnX    1,B
#define CLGLCD_FLM         D,1

#define CLGLCD_TP_XL_AN    A8
#define CLGLCD_TP_XL       B,4
#define CLGLCD_TP_YU       E,6
#define CLGLCD_TP_XR       D,7
#define CLGLCD_TP_YL_AN    A6
#define CLGLCD_TP_YL       D,4

#else 
  #error "No config for this board"
#endif // Board config

#define CLGLCD_CL1_TICKS    952

//
// Touchpanel calibration
//

#define CLGLCD_TP_CAL_XF     378 
#define CLGLCD_TP_CAL_XM     76
#define CLGLCD_TP_CAL_YF     308
#define CLGLCD_TP_CAL_YM     107
#define CLGLCD_TP_CAL_Z1_MIN 5
#define CLGLCD_TP_CAL_Z1_MAX 70
#define CLGLCD_TP_CAL_Z2_MAX 124
#define CLGLCD_TP_CAL_Z2_MIN 60

//
// Poweroff display after ~3sec of no data
//
//#define CLGLCD_DISPOFF_CUT  50420
#define CLGLCD_DISPOFF_CUT  480

//
// Ammount of data to buffer, each bank is 20 bytes
//
#define CLGLCD_BUFFER_BANKS 48

#endif //CLGLCD_CONFIG_H
