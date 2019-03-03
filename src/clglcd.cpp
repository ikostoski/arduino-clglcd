/*
 * Character generator for Controllerless Graphics LCD Display
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

#include "clglcd.h"

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

#define TM_BASE_(n,x) (n)
#define TM_BASE(cfg)  TM_BASE_(cfg)
#define TM_LINE_A     1
#define TM_LINE_B     2
#define TM_LINE_C     3
#define TM_LINE_D     4
#define TM_LINE_(n,x) (TM_LINE_ ## x)
#define TM_LINE(cfg)  TM_LINE_(cfg)
#define TM_CR_(n,x)   (TCCR ## n ## x)
#define TM_CR(cfg)    TM_CR_(cfg)
#define TM_OCR_(n,x)  (OCR ## n ## x)
#define TM_OCR(cfg)   TM_OCR_(cfg)

#define TM_FPWM_MODE2_(n,x)     (1<<WGM ## n ## 1) 
#define TM_FPWM_MODE2(cfg)      TM_FPWM_MODE2_(cfg)
#define TM_FPWM_MODE3_(n,x)     (1<<WGM ## n ## 1) | (1<<WGM ## n ## 0) 
#define TM_FPWM_MODE3(cfg)      TM_FPWM_MODE3_(cfg)
#define TM_FPWM_TOGGLE_(n,x)    (1<<COM ## n ## x ## 0)
#define TM_FPWM_TOGGLE(cfg)     TM_FPWM_TOGGLE_(cfg)
#define TM_FPWM_CLEAR_(n,x)     (1<<COM ## n ## x ## 1)
#define TM_FPWM_CLEAR(cfg)      TM_FPWM_CLEAR_(cfg)
#define TM_FPWM_SET_(n,x)       (1<<COM ## n ## x ## 1) | (1<<COM ## n ## x ## 0)
#define TM_FPWM_SET(cfg)        TM_FPWM_SET_(cfg)


#define SET_BIT(p,b)    ((p) |= _BV(b))
#define CLEAR_BIT(p,b)  ((p) &= ~_BV(b))
#define TOGGLE_BIT(p,b) ((p) ^= _BV(b))
#define OUTPUT_PIN(pin) DDR_(pin) |= _BV(BIT_(pin))

// 952 Fclk ticks @ 16MHz ~= 59.5us 
#define CLGLCD_CL1_TICKS  952

#if CLGLCD_SOFT_CHARS < 1
#define CLGLCD_CL2_TICKS  4
#else
#define CLGLCD_CL2_TICKS  6
#endif

// Timers...
#if (TM_BASE(CLGLCD_CL1_OCnX) == 1) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 1)
  #define CL1_TIMER_CR  TCCR1A
  #define CL1_PULSE     TM_FPWM_SET(CLGLCD_CL1_OCnX) | TM_FPWM_MODE2(CLGLCD_CL1_OCnX) 
  #define CL1_SET_M     CL1_PULSE | TM_FPWM_SET(CLGLCD_ALT_M_OCnX) 
  #define CL1_CLEAR_M   CL1_PULSE | TM_FPWM_CLEAR(CLGLCD_ALT_M_OCnX)
#elif (TM_BASE(CLGLCD_CL1_OCnX) == 4) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 4)
  #define CL1_TIMER_CR  TCCR4C
  #define CL1_PULSE     TM_FPWM_SET(CLGLCD_CL1_OCnX) | (1<<PWM4D)
  #define CL1_SET_M     CL1_PULSE | TM_FPWM_SET(CLGLCD_ALT_M_OCnX) 
  #define CL1_CLEAR_M   CL1_PULSE | TM_FPWM_CLEAR(CLGLCD_ALT_M_OCnX)
#else
  error "CL1 timer not implemented"
#endif

#if TM_BASE(CLGLCD_CL2_OCnX) == 1
  #define CL2_TIMER_CR    TCCR1B
  #define CL2_TIMER_CNT   TCNT1L
  #define CL2_TIMER_VAL   (1<<WGM13)|(1<<WGM12)|(1<<CS10)
#elif TM_BASE(CLGLCD_CL2_OCnX) == 2
  #define CL2_TIMER_CR    TCCR2B
  #define CL2_TIMER_CNT   TCNT2
  #define CL2_TIMER_VAL   ((1<<WGM22)|(1<<CS20))
#elif TM_BASE(CLGLCD_CL2_OCnX) == 3
  #define CL2_TIMER_CR    TCCR3B
  #define CL2_TIMER_CNT   TCNT3L
  #define CL2_TIMER_VAL   (1<<WGM33)|(1<<WGM32)|(1<<CS10)
#elif TM_BASE(CLGLCD_CL2_OCnX) == 4
  #define CL2_TIMER_CR    TCCR4B
  #define CL2_TIMER_CNT   TCNT4
  #define CL2_TIMER_VAL   (1<<PSR4)|(1<<CS40)
  #define CL2_TIMER_SYNC() "nop\n\tnop\n\t"
#else
  error "CL2 timer not implemented"
#endif

#if !defined CL2_TIMER_SYNC
  #define CL2_TIMER_SYNC()  "nop\n\t"
#endif

//
// Globlas
// 

uint8_t volatile screen[CLGLCD_Y_LINES][40]; 

static uint16_t volatile screen_pos;
static volatile uint8_t font_line;

#if CLGLCD_SOFT_CHARS > 0
uint8_t volatile soft_font[CLGLCD_FONT_LINES][CLGLCD_SOFT_CHARS] __attribute__((aligned(CLGLCD_SOFT_CHARS))); 
#endif

//
// Character generator ISR
//

#if CLGLCD_SOFT_CHARS < 1 // Fixed

// Outputs 2 nibbles in 8 Fclk ticks 
#define SHIFT_OUT_BYTE() \
    "out %[data_port], r24" "\n\t" \
    "ld  r30, X+" "\n\t" \
    "swap r24" "\n\t"   \
    "out %[data_port], r24" "\n\t" \
    "lpm r24, Z\n\t" 
    
#else // Fixed + Soft

#define SHIFT_OUT_BYTE_SCR_LOAD() \
    "ld  r30, X+" "\n\t" \

#if CLGLCD_SOFT_UPPER == 1
#define SHIFT_OUT_BYTE_BRANCH_3F() \
    "cpi r30, %[sfc_upper]" "\n\t" \
    "brcc 3f" "\n\t"
#else 
#define SHIFT_OUT_BYTE_BRANCH_3F() \
    "cpi r30, %[sfc_num]" "\n\t" \
    "brcs 3f" "\n\t"
#endif

#define SHIFT_OUT_BYTE_FIXED_LOAD() \
    "lpm r28, Z" "\n\t" \

#if CLGLCD_SOFT_UPPER == 1
#define SHIFT_OUT_BYTE_SOFT_OFFSET() \
    "mov r28, r30" "\n\t" \
    "and r28, r0" "\n\t"
#else 
#define SHIFT_OUT_BYTE_SOFT_OFFSET() \
    "mov r28, r30" "\n\t" \
    "or r28, r0" "\n\t"
#endif 

#define SHIFT_OUT_BYTE_SOFT_LOAD() \
    "ld r28, Y" "\n\t" 

// Outputs 2 nibbles in 12 Fclk ticks
#define SHIFT_OUT_BYTE() \
    "out %[data_port], r28" "\n\t" \
    SHIFT_OUT_BYTE_SCR_LOAD() \
    "swap r28" "\n\t" \
    SHIFT_OUT_BYTE_BRANCH_3F() \
    "out %[data_port], r28" "\n\t" \
    SHIFT_OUT_BYTE_FIXED_LOAD() \
    "rjmp 4f" "\n\t" \
  "3:" \
    /* We are a tick late with this nibble */ \
    "out %[data_port], r28" "\n\t" \
    SHIFT_OUT_BYTE_SOFT_OFFSET() \
    SHIFT_OUT_BYTE_SOFT_LOAD() \
  "4:" 

#endif

// Common shifting macros
#define SHIFT_OUT_3X_BYTE() \
  SHIFT_OUT_BYTE() \
  SHIFT_OUT_BYTE() \
  SHIFT_OUT_BYTE()
  
#define SHIFT_OUT_9X_BYTE() \
  SHIFT_OUT_3X_BYTE() \
  SHIFT_OUT_3X_BYTE() \
  SHIFT_OUT_3X_BYTE()

// Set vector based on config
#if TM_BASE(CLGLCD_CL1_OCnX) == 1
ISR(TIMER1_OVF_vect, ISR_NAKED) {  
#elif TM_BASE(CLGLCD_CL1_OCnX) == 2
ISR(TIMER2_OVF_vect, ISR_NAKED) {
#elif TM_BASE(CLGLCD_CL1_OCnX) == 3
ISR(TIMER3_OVF_vect, ISR_NAKED) {
#elif TM_BASE(CLGLCD_CL1_OCnX) == 4
ISR(TIMER4_OVF_vect, ISR_NAKED) {
#else  
#error "CL1 timer not implemented"
#endif
  __asm__ (
    ".equ font_end, %[font_start]+%[font_size]" "\n\t"
    ".equ screen_start, %[screen]" "\n\t" 
    ".equ screen_end, %[screen]+%[screen_size]" "\n\t"
    #if CLGLCD_SOFT_CHARS > 0     
    ".equ soft_offset, %[soft_font]" "\n\t" 
    #endif

    "push r24" "\n\t"   
    "in r24, 0x3f" "\n\t"
    "push r24" "\n\t"
    "push r26" "\n\t"
    "push r27" "\n\t"
    "push r30" "\n\t"
    "push r31\n\t"
    #if CLGLCD_SOFT_CHARS > 0
    "push r0" "\n\t"
    "push r1" "\n\t"
    "push r28" "\n\t"
    "push r29" "\n\t"
    #endif

    // Clear FLM
    "cbi %[flm_port], %[flm_bit]" "\n\t"
    
  "1:"        
    // Pickup where we left off
    "lds r31, %[font_line]" "\n\t"
    "lds r26, %[scr_pos]" "\n\t"
    "lds r27, %[scr_pos]+1" "\n\t"

    // Check if we need to go to top of screen
    "ldi r30, hi8(screen_end)" "\n\t"
    "cpi r26, lo8(screen_end)" "\n\t"
    "cpc r27, r30" "\n\t"
    "brcs 2f" "\n\t"
      // We have sent all screen lines, ...
      // ... reset screen pointer, ...
      "ldi r26, lo8(screen_start)" "\n\t"
      "ldi r27, hi8(screen_start)" "\n\t"
      "sts %[scr_pos], r26" "\n\t"
      "sts %[scr_pos]+1, r27\n\t"
      // ... reset font line, ...
      "ldi r31, hi8(%[font_start])" "\n\t"
      // ... set FLM line UP, ...
      "sbi %[flm_port], %[flm_bit]" "\n\t"
      // ... and set ALT_M_OCnX to be flipped on
      // next CL1 timer match.
      "ldi r24, %[cl1_set_m]" "\n\t"
      "sbis %[alt_m_portin], %[alt_m_bit]" "\n\t"
      "ldi r24, %[cl1_clear_m]" "\n\t"
      "sts %[cl1_tccr], r24" "\n\t" 
            
  "2:"
    #if CLGLCD_SOFT_CHARS < 1 // Fixed
    // Preload first byte in r24
    "ld  r30, X+" "\n\t" 
    "lpm r24, Z" "\n\t" 
    #else // Soft + Fixed
    // Calculate YH and YL top bits from r31 font line
    "mov r29, r31" "\n\t"
    "subi r29, hi8(%[font_start])" "\n\t" 
    "ldi r28, %[sfc_num]" "\n\t"
    "mul r28, r29" "\n\t"
    "ldi r28, lo8(soft_offset)" "\n\t"
    "ldi r29, hi8(soft_offset)" "\n\t"
    "add r0, r28" "\n\t"
    #if CLGLCD_SOFT_UPPER == 1
    "ldi r28, %[sfc_mask]\n\t"
    "or r0, r28" "\n\t"
    #endif
    "adc r29, r1" "\n\t"
    // Keep r0 as now it contains bitmask offset to soft font data
    
    // Preload first byte in r28
    SHIFT_OUT_BYTE_SCR_LOAD() 
    SHIFT_OUT_BYTE_BRANCH_3F()
    SHIFT_OUT_BYTE_FIXED_LOAD()
    "rjmp 4f" "\n\t"
  "3:"
    SHIFT_OUT_BYTE_SOFT_OFFSET()
    SHIFT_OUT_BYTE_SOFT_LOAD()
  "4:"
    #endif // Fixed/Soft
    
    // Start timer
    "eor r30, r30" "\n\t"
    "sts %[cl2_tcnt], r30\n\t"
    "ldi r30, %[cl2_tccr_val]" "\n\t"
    "sts %[cl2_tccr], r30" "\n\t"
    CL2_TIMER_SYNC()

    // Shift out 39 (4x9+3) bytes
    SHIFT_OUT_9X_BYTE()
    SHIFT_OUT_9X_BYTE()
    SHIFT_OUT_9X_BYTE()
    SHIFT_OUT_9X_BYTE()
    SHIFT_OUT_3X_BYTE()
  
    // And for the last byte:
    // - Do not load next screen byte
    // - Stop timer ASAP,

    #if CLGLCD_SOFT_CHARS < 1 // Fixed font 
        
    "out %[data_port], r24" "\n\t" 
    "swap r24" "\n\t"
    "eor r30, r30" "\n\t"
    "nop" "\n\t"
    "out %[data_port], r24" "\n\t"
    // Stop the timer just as CL2 goes down (STS takes 2 cycles)  
    "sts %[cl2_tccr], r30" "\n\t"
    
    #else // Fixed+Soft
    
    "out %[data_port], r28" "\n\t"
    "swap r28" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    //"ldi r24, %[cl1_bv]" "\n\t"
    "nop" "\n\t"
    "nop" "\n\t"
    // Output last nibble
    "out %[data_port], r28" "\n\t"
    // Stop the timer just as CL2 goes down (STS takes 2 cycles)  
    "nop" "\n\t"
    "eor r1, r1" "\n\t"
    "sts %[cl2_tccr], r1" "\n\t"
    
    #endif // Fixed/Soft
         
    "subi r31, 0xFF" "\n\t"
    "cpi r31, hi8(font_end)" "\n\t"
    "brcs 5f" "\n\t"
      // We have sent one full screen line (CLGLCD_FONT_SIZE)
      // Store screen position (in X register), for next interrupt
      "sts %[scr_pos], r26" "\n\t"
      "sts %[scr_pos]+1, r27\n\t"
      // Rreset the font line
      "ldi r31, hi8(%[font_start])" "\n\t"
  "5:"
    // Store current font line for next interrupt
    "sts %[font_line], r31\n\t"
      
    #if CLGLCD_SOFT_CHARS > 0
    "pop r29" "\n\t"
    "pop r28" "\n\t"
    "pop r1" "\n\t"
    "pop r0" "\n\t"
    #endif
    "pop r31" "\n\t"
    "pop r30" "\n\t"
    "pop r27" "\n\t"
    "pop r26" "\n\t"
    "pop r24" "\n\t"
    "out 0x3f, r24" "\n\t"    
    "pop r24" "\n\t"
    "reti" "\n\t"
    
    :: 
      [font_start] "" (fixed_font),
      [font_size] "" (256 * CLGLCD_FONT_LINES),
      [screen] "" (screen),
      [screen_size] "" (40 * CLGLCD_Y_LINES),
      [scr_pos] "" (screen_pos),
      [font_line] "" (font_line),
      #if CLGLCD_SOFT_CHARS > 0     
        [soft_font] "" (soft_font),
        [sfc_num] "" (CLGLCD_SOFT_CHARS),
        [sfc_mask] "" (CLGLCD_SOFT_CHARS-1),
        [sfc_upper] "" (256-CLGLCD_SOFT_CHARS),
      #endif
      [cl2_tcnt] "" (CL2_TIMER_CNT),
      [cl2_tccr_val] "" (CL2_TIMER_VAL),
      [cl2_tccr] "" (&CL2_TIMER_CR),
      [cl1_set_m] "" (CL1_SET_M),
      [cl1_clear_m] "" (CL1_CLEAR_M),
      [cl1_tccr] "" (&CL1_TIMER_CR),
      [data_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_DATA))),
      [flm_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_FLM))),
      [flm_bit] "" (BIT(CLGLCD_FLM)),
      [alt_m_port] "" (_SFR_IO_ADDR(PORT(CLGLCD_ALT_M))),
      [alt_m_portin] "" (_SFR_IO_ADDR(PIN(CLGLCD_ALT_M))),
      [alt_m_bit] "" (BIT(CLGLCD_ALT_M))
  );
}

//
// CLGLCD API
//

void CLGLCD_init() {
  CLEAR(CLGLCD_DISPOFF);
  OUTPUT_PIN(CLGLCD_DISPOFF);
  #if defined(CLGLCD_BCKL_CTRL)
  CLEAR(CLGLCD_BCKL_CTRL);
  OUTPUT_PIN(CLGLCD_BCKL_CTRL);
  #endif
  #if defined(CLGLCD_VEE_CTRL)
  CLEAR(CLGLCD_VEE_CTRL);
  OUTPUT_PIN(CLGLCD_VEE_CTRL);
  #endif

  DDR(CLGLCD_DATA) = 0xF0;
  OUTPUT_PIN(CLGLCD_ALT_M);  
  OUTPUT_PIN(CLGLCD_CL2);
  OUTPUT_PIN(CLGLCD_CL1);
  OUTPUT_PIN(CLGLCD_FLM);

  #if CLGLCD_SOFT_CHARS > 0
    for(uint8_t y=0; y<CLGLCD_FONT_LINES; y++) {
      #if CLGLCD_SOFT_UPPER == 1
        memcpy_P((void *)&soft_font[y], &fixed_font[y*256+256-CLGLCD_SOFT_CHARS], CLGLCD_SOFT_CHARS);
      #else 
        memcpy_P((void *)&soft_font[y], &fixed_font[y*256], CLGLCD_SOFT_CHARS);
      #endif
    }
  #endif  
}

void CLGLCD_on() {

  noInterrupts();
  screen_pos = (uint16_t)&screen;
  font_line = ((uint16_t)&fixed_font >> 8);       

  // Shift out the row selector bit (FLM) from common drivers, 
  // so we don't end up with two active bits 
  CLEAR(CLGLCD_CL1);
  CLEAR(CLGLCD_FLM);
  for (uint8_t y=0; y<240; y++) {
    TOGGLE(CLGLCD_CL1);
    _NOP();
    TOGGLE(CLGLCD_CL1);
  }  
  
  // Setup the CL1 timer 
  #if (TM_BASE(CLGLCD_CL1_OCnX) == 1) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 1)
    TCCR1A = 0;
    TCCR1B = 0; 
    TCNT1  = 0;  
    ICR1   = CLGLCD_CL1_TICKS / 8 - 1; 
    TM_OCR(CLGLCD_ALT_M_OCnX) = CLGLCD_CL1_TICKS / 8 - 1; 
    TM_OCR(CLGLCD_CL1_OCnX) = CLGLCD_CL1_TICKS / 8 - 2; 
    TCCR1A = CL1_CLEAR_M;
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11); // WGM13:0=15, prescaller 8
    TIFR1  |= (1 << TOV1);
    TIMSK1 |= (1 << TOIE1);  // enable timer overflow interrupt
  #elif (TM_BASE(CLGLCD_CL1_OCnX) == 4 ) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 4)
    TIMSK4 = 0;     
    TCCR4A = 0; 
    TCCR4B = 0; 
    TCCR4C = 0; 
    TCCR4D = 0; 
    TCCR4E = 0; 
    TC4H   = 0;
    TCNT4  = 0; 
    //TC4H   = highByte(CLGLCD_CL1_TICKS / 8 - 1);
    OCR4C  = lowByte(CLGLCD_CL1_TICKS / 8 - 1); // TOP
    TM_OCR(CLGLCD_ALT_M_OCnX) = lowByte(CLGLCD_CL1_TICKS / 8); 
    TM_OCR(CLGLCD_CL1_OCnX) = lowByte(CLGLCD_CL1_TICKS / 8 - 2); 
    TCCR4A = (1<<PWM4A) | (1<<PWM4B);
    TCCR4B = (1<<CS42); // Prescaller 8
    TCCR4C = CL1_CLEAR_M;
    TIFR4  |= (1 << TOV4);
    TIMSK4 |= (1 << TOIE4);  // enable timer overflow interrupt
  #else
    #error "CL1 timer not implemented"
  #endif

  // Setup the CL2 Timer 
  #if TM_BASE(CLGLCD_CL2_OCnX) == 1
    TIMSK1 = 0;
    TCCR1A = 0; 
    TCCR1B = 0;
    TCNT1  = 0;
    ICR1   = CLGLCD_CL2_TICKS - 1; 
    TM_OCR(CLGLCD_CL2_OCnX) = CLGLCD_CL2_TICKS / 2 - 1
    TCCR1A = TM_FPWM_SET(CLGLCD_CL2_OCnX);
  #elif TM_BASE(CLGLCD_CL2_OCnX) == 2
    #if TM_LINE(CLGLCD_CL2_OCnX) == TM_LINE_B
      TIMSK2 = 0;
      TCCR2A = 0;
      TCCR2B = 0;
      TCNT2  = 0;
      OCR2A  = CLGLCD_CL2_TICKS - 1; 
      TM_OCR(CLGLCD_CL2_OCnX) = CLGLCD_CL2_TICKS / 2 - 1;
      TCCR2A = ((1<<COM2B1) | (1<<COM2B0) | (1<<WGM21) | (1<<WGM20));
    #else 
      #error Unsupported CL2 Timer2 configuration
    #endif
  #elif (TM_BASE(CLGLCD_CL2_OCnX) == 3) && defined(__AVR_ATmega32U4__)
    #if TM_LINE(CLGLCD_CL2_OCnX) == TM_LINE_A
      TIMSK3 = 0;
      TCCR3A = 0; 
      TCCR3B = 0;
      TCNT3  = 0;
      //ICR3   = CLGLCD_CL2_TICKS - 1; 
      TM_OCR(CLGLCD_CL2_OCnX) = CLGLCD_CL2_TICKS / 2 - 1;
      TCCR3A = TM_FPWM_TOGGLE(CLGLCD_CL2_OCnX) | TM_FPWM_MODE3(CLGLCD_CL2_OCnX);
    #else 
      #error Unsupported CL2 Timer2 configuration
    #endif
  #elif (TM_BASE(CLGLCD_CL2_OCnX) == 4) && defined(__AVR_ATmega32U4__)
    // Hi-Speed Timer 4 needs special treatment, even in Sync mode
    TIMSK4 = 0; 
    TCCR4A = 0; 
    TCCR4B = 0; 
    TCCR4C = 0; 
    TCCR4D = 0; 
    TC4H   = 0; 
    TCNT4  = 0; 
    OCR4C  = CLGLCD_CL2_TICKS - 1;
    TM_OCR(CLGLCD_CL2_OCnX) = CLGLCD_CL2_TICKS/2 - 1;
    TCCR4A = (1<<PWM4B) | (1<<PWM4A);
    TCCR4C = TM_FPWM_SET(CLGLCD_CL2_OCnX) | (1<<PWM4D);
  #else
    #error "Unsupported CL2 timer configuration"
  #endif

  interrupts();
    
  // Startup sequence

  #if defined(CLGLCD_VEE_CTRL)
  delayMicroseconds(60);
  SET(CLGLCD_VEE_CTRL);
  #endif  
    
  delayMicroseconds(60);

  #if defined(CLGLCD_BCKL_CTRL)
  SET(CLGLCD_BCKL_CTRL);
  #endif
  
  SET(CLGLCD_DISPOFF);
}

void CLGLCD_off() {
  // Shutdown sequence
  CLEAR(CLGLCD_DISPOFF);

  #if defined(CLGLCD_BCKL_CTRL)
  CLEAR(CLGLCD_BCKL_CTRL);
  #endif

  delayMicroseconds(60);
  
  #if defined(CLGLCD_VEE_CTRL)
  CLEAR(CLGLCD_VEE_CTRL);
  delayMicroseconds(60);  
  #endif  

  // Stop driving interrupt
  #if TM_BASE(CLGLCD_CL1_OCnX) == 1
    TIMSK1 &= ~(1 << TOIE1);    
  #elif TM_BASE(CLGLCD_CL1_OCnX) == 2
    TIMSK4 &= ~(1 << TOIE4);    
  #elif TM_BASE(CLGLCD_CL1_OCnX) == 3
    TIMSK4 &= ~(1 << TOIE4);    
  #elif TM_BASE(CLGLCD_CL1_OCnX) == 4
    TIMSK4 &= ~(1 << TOIE4);    
  #endif
}

void CLGLCD_clear_screen() {
  memset((void *)&screen, 0, sizeof(screen));     
}

bool CLGLCD_FLM_is_up() {
  return (PORT(CLGLCD_FLM) & BIT(CLGLCD_FLM));
}
