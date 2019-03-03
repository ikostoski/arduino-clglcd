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
//

#if defined(ARDUINO_AVR_UNO)

#define CLGLCD_BCKL_CTRL   B,5
#define CLGLCD_DISPOFF     B,4
#define CLGLCD_DATA        D,4-7
#define CLGLCD_VEE_CTRL    B,3
#define CLGLCD_ALT_M       B,1
#define CLGLCD_ALT_M_OCnX  1,A
#define CLGLCD_CL2         D,3
#define CLGLCD_CL2_OCnX    2,B
#define CLGLCD_CL1         B,2
#define CLGLCD_CL1_OCnX    1,B
#define CLGLCD_FLM         B,0

#elif defined(ARDUINO_AVR_LEONARDO)

#define CLGLCD_BCKL_CTRL   D,6
#define CLGLCD_DISPOFF     D,3
#define CLGLCD_DATA        B,4-7
#define CLGLCD_VEE_CTRL    E,6
#define CLGLCD_ALT_M       D,7
#define CLGLCD_ALT_M_OCnX  4,D
#define CLGLCD_CL2         C,6
#define CLGLCD_CL2_OCnX    3,A
#define CLGLCD_CL1         C,7
#define CLGLCD_CL1_OCnX    4,A
#define CLGLCD_FLM         D,4

#elif defined(ARDUINO_AVR_PROMICRO)

#define CLGLCD_BCKL_CTRL   D,2
#define CLGLCD_DISPOFF     D,3
#define CLGLCD_DATA        F,4-7
#define CLGLCD_VEE_CTRL    E,6
#define CLGLCD_ALT_M       B,5
#define CLGLCD_ALT_M_OCnX  1,A
#define CLGLCD_CL2         C,6
#define CLGLCD_CL2_OCnX    3,A
#define CLGLCD_CL1         B,6
#define CLGLCD_CL1_OCnX    1,B
#define CLGLCD_FLM         B,2

#else 
  #error "No config for this board"
#endif // Board config

// Define the number of soft (runtime modifiable) characters 
// NOTE: Defining soft characters will eat SRAM and 
//       slow down main code significantly
// 
// Muse be power of 2. Supported values 8, 16, 32, 64
//#define CLGLCD_SOFT_CHARS 32

// Define to use upper codes (255-CLGLCD_SOFT_CHARS..255)
// otherwise it it will use (0..CLGLCD_SOFT_CHARS)
//#define CLGLCD_SOFT_UPPER 1

#endif //CLGLCD_CONFIG
