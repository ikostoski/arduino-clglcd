/*
 * ATmega32u4 USB bridge for Controllerless Graphics LCD Display
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

#if !defined(__AVR_ATmega32U4__)
  error "Works only on ATmega32u4 MCU"
#endif

#include "clglcd_config.h"
#include "clglcd_macros.h"
#include "clglcd_usb_wcid.h"

#ifndef CLGLCD_BUFFER_BANKS
#define CLGLCD_BUFFER_BANKS 24
#endif

//See comment inside setup function
#define DISABLE_USB_CORE_IRQ 

#if (TM_BASE(CLGLCD_CL1_OCnX) == 1) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 1)
  #define CL1_TIMER_CR  TCCR1A
  #define CL1_BASE      TM_WGM(CLGLCD_CL1_OCnX, 1) | TM_WGM(CLGLCD_CL1_OCnX, 0)
#elif (TM_BASE(CLGLCD_CL1_OCnX) == 4) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 4)
  #define CL1_TIMER_CR  TCCR4C
  #define CL1_BASE      0 //(1<<PWM4D)
#else
  #error "Unsupported CL1 timer configuration"
#endif
#define CL1_SET       TM_COM_SET(CLGLCD_CL1_OCnX) 
#define CL1_SET_M     TM_COM_SET(CLGLCD_ALT_M_OCnX)
#define CL1_CLEAR_M   TM_COM_CLEAR(CLGLCD_ALT_M_OCnX)
#define CL1_TOGGLE_M  TM_COM_TOGGLE(CLGLCD_ALT_M_OCnX)
  

#define CL2_START  TM_WGM(CLGLCD_CL2_OCnX, 3) | TM_WGM(CLGLCD_CL2_OCnX, 2) | TM_PSC1(CLGLCD_CL2_OCnX)
#define CL2_STOP   TM_WGM(CLGLCD_CL2_OCnX, 3) | TM_WGM(CLGLCD_CL2_OCnX, 2)

CLGLCD_USB_ USB;

static volatile uint16_t buf_head = 0;
static volatile uint16_t buf_tail = 0;
static volatile uint16_t power_cnt = 0;
static volatile uint8_t buf_cnt;
static volatile uint8_t row_cnt;
static volatile uint8_t lcd_ep;
static volatile uint8_t lcd_buf[20 * CLGLCD_BUFFER_BANKS]; 


void setup() {
  CLEAR(CLGLCD_DISPOFF);
  OUTPUT_PIN(CLGLCD_DISPOFF);
  CLEAR(CLGLCD_VEE_CTRL);
  OUTPUT_PIN(CLGLCD_VEE_CTRL);
  CLEAR(CLGLCD_BCKL_CTRL);
  OUTPUT_PIN(CLGLCD_BCKL_CTRL);

  delay(1000); // Safety
  
  DDR(CLGLCD_DATA) = 0xF0;    
  OUTPUT_PIN(CLGLCD_ALT_M);  
  OUTPUT_PIN(CLGLCD_CL2);
  OUTPUT_PIN(CLGLCD_CL1);
  OUTPUT_PIN(CLGLCD_FLM);
  

  noInterrupts();

  lcd_ep = USB.getEP();
  buf_head = (uint16_t)&lcd_buf;
  buf_tail = (uint16_t)&lcd_buf;
  power_cnt = 0;
  buf_cnt = 0;
  row_cnt = 239;
  
  // Setup the CL2 Timer @ Fclk/3
  #if (TM_BASE(CLGLCD_CL2_OCnX) == 1) || (TM_BASE(CLGLCD_CL2_OCnX) == 3)
    TM_IMSK(CLGLCD_CL2_OCnX)  = 0;
    TM_CR(CLGLCD_CL2_OCnX,A)  = 0;
    TM_CR(CLGLCD_CL2_OCnX,B)  = 0;
    TM_CNT(CLGLCD_CL2_OCnX)   = 0;
    TM_ICR(CLGLCD_CL2_OCnX)   = 2;
    TM_OCR(CLGLCD_CL2_OCnX)   = 1;
    // FastPWM Mode 14
    TM_CR(CLGLCD_CL2_OCnX, A) = TM_COM_SET(CLGLCD_CL2_OCnX) | TM_WGM(CLGLCD_CL2_OCnX, 1);
    // Starting and stoping here seems to help start CL2 inside ISR ?
    TM_CR(CLGLCD_CL2_OCnX, B) = CL2_START;
    TM_CR(CLGLCD_CL2_OCnX, B) = CL2_STOP;
  #else
    #error "Unsupported CL2 timer configuration"
  #endif  

  // Shift out the row selector bit (FLM) from common drivers, 
  // so we don't end up with two active bits 
  CLEAR(CLGLCD_CL1);
  CLEAR(CLGLCD_FLM);  
  for (uint8_t y=0; y<240; y++) {
    TOGGLE(CLGLCD_CL1);
    _NOP();
    TOGGLE(CLGLCD_CL1);
  }  
  
  // Setup CL1 Timer and start it
  #if (TM_BASE(CLGLCD_CL1_OCnX) == 1) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 1)
    TIMSK1 = 0;
    TCCR1A = 0;
    TCCR1B = 0; 
    TCNT1  = 0;  
    ICR1   = CLGLCD_CL1_TICKS - 1; 
    TM_OCR(CLGLCD_ALT_M_OCnX) = CLGLCD_CL1_TICKS - 1; 
    TM_OCR(CLGLCD_CL1_OCnX) = CLGLCD_CL1_TICKS - 3; 
    TCCR1A = CL1_BASE | CL1_SET | CL1_SET_M;
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS10); // WGM13:0=15, prescaller 8
    TIFR1  |= (1 << TOV1);
    TIMSK1 |= (1 << TOIE1);  // enable timer overflow interrupt    
  #elif (TM_BASE(CLGLCD_CL1_OCnX) == 4) && (TM_BASE(CLGLCD_ALT_M_OCnX) == 4)
    TIMSK4 = 0;     
    TCCR4A = 0; 
    TCCR4B = 0; 
    TCCR4C = 0; 
    TCCR4D = 0; 
    TCCR4E = 0; 
    TC4H   = 0;
    TCNT4  = 0;
    TC4H   = highByte(CLGLCD_CL1_TICKS - 1);
    OCR4C  = lowByte(CLGLCD_CL1_TICKS - 1); // TOP
    TM_OCR(CLGLCD_ALT_M_OCnX) = lowByte(CLGLCD_CL1_TICKS - 1); 
    TM_OCR(CLGLCD_CL1_OCnX) = lowByte(CLGLCD_CL1_TICKS - 3); 
    TCCR4A = (1<<PWM4A) | (1<<PWM4B);
    TCCR4B = (1<<CS40); // Prescaller 1
    TCCR4C = CL1_BASE | CL1_SET | CL1_SET_M;
    TIFR4  |= (1 << TOV4);
    TIMSK4 |= (1 << TOIE4);  // enable timer overflow interrupt
  #else
    #error "Unsupported CL1 timer configuration"
  #endif

  // Disable USB_Core interrupts and call them manually 
  // in the main loop. 
  // USB_Core is written in a way that it allways clobbers 
  // UENUM register, forcing main loop code to disable interrputs 
  // even for basic reading of any USB endpoint registers. 
  // Another side effect is that PluggableUSB routines can
  // take very long time (i.e. sending the USB descriptor)
  // with interrupts disabled. This leads to visual artifacts
  // on the LCD (where we need interrput every 59.5us). 
  // So lets try to work arrond that...
  #ifdef DISABLE_USB_CORE_IRQ
    // Disable endpoint 0 interrupt
    UENUM = 0;
    UEIENX &= ~(1<<RXSTPE);
    // Disable device level interrupts
    UDIEN = 0;
  #endif
    
  interrupts();  
}

// 
// Macros used to shift data from memory to LCD column drivers. 
// CL2 clock will be provider by PWM from timer which needs 
// Fclk/3 period (5.33MHz on 16MHz AVR) and 33%/67% duty cycle.
// The block takes exacly 6 Fclk ticks, sending 2x4 bits 
// of data to D0-D3 lines
//

#define SHIFT_BYTE_FROM_BUFFER() \
    "out %[data_port], r24" "\n\t" \
    "swap r24"           "\n\t" \
    "nop"                "\n\t" \
    "out %[data_port], r24" "\n\t" \
    "ld r24, X+"         "\n\t"

#define SHIFT_3X_BYTE_FROM_BUFFER() \
    SHIFT_BYTE_FROM_BUFFER() \
    SHIFT_BYTE_FROM_BUFFER() \
    SHIFT_BYTE_FROM_BUFFER()

#define SHIFT_9X_BYTE_FROM_BUFFER() \
    SHIFT_3X_BYTE_FROM_BUFFER() \
    SHIFT_3X_BYTE_FROM_BUFFER() \
    SHIFT_3X_BYTE_FROM_BUFFER()
  
#define MOVE_USB_BYTE_TO_BUFFER() \
    "ldd r24, Z+%[uedatx_ofs]" "\n\t" \
    "st X+, r24" "\n\t" 
  
#define MOVE_10x_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER() \
    MOVE_USB_BYTE_TO_BUFFER()

#if TM_BASE(CLGLCD_CL1_OCnX) == 1
ISR(TIMER1_OVF_vect, ISR_NAKED) {  
#elif TM_BASE(CLGLCD_CL1_OCnX) == 4
ISR(TIMER4_OVF_vect, ISR_NAKED) {
#else  
  #error "CL1 timer not implemented"
#endif
  __asm__ (
    ".equ buf_start, %[buf_start]" "\n\t" 
    ".equ buf_end, buf_start+%[buf_size]" "\n\t"
    ".equ dispoff_cut, %[dispoff_cut]" "\n\t"
    ".equ vee_cut, %[dispoff_cut]+1" "\n\t"
    :: [buf_start] "" (&lcd_buf),
       [buf_size] "" (20 * CLGLCD_BUFFER_BANKS),
       [dispoff_cut] "" (CLGLCD_DISPOFF_CUT)
  );

  __asm__ (
    "push r24" "\n\t"
    "in r24, __SREG__" "\n\t"
    "push r24" "\n\t"
    "push r25" "\n\t"
    "push r26" "\n\t"
    "push r27" "\n\t"
    "push r30" "\n\t"
    "push r31" "\n\t"

    // Clear FLM
    "cbi %[flm_port], %[flm_bit]" "\n\t"    

    // Load buffer count
    "lds r25, %[buf_cnt]" "\n\t"        
    "subi r25, 0x02\n\t" // Decrement (borrow) 2 banks (40 bytes)    
    "brpl buf2lcd" "\n\t" // If counter is still postive, output data

    // Couunter is negative, we do not output data    
    "subi r25, 0xFE\n\t" // Return what we borrowed       
    "brpl buf_undr" "\n\t" // If we are above zero, it is buffer underrun    

    // Else we are waiting/loading usb data
    // Check power state
    "sbis %[vee_port], %[vee_bit]" "\n\t"
    "rjmp usb_ep" "\n\t" // Display is already OFF
    
    // Display is ON and we are not shifting data, 
    // limit the time we spend in this state
    "lds r26, %[power_cnt]" "\n\t"
    "lds r27, %[power_cnt]+1" "\n\t"
    "adiw r26, 0x01" "\n\t"
    "ldi r24, hi8(dispoff_cut)" "\n\t"
    "cpi r26, lo8(dispoff_cut)" "\n\t"
    "cpc r27, r24" "\n\t"
    "brlo 1f" "\n\t"
    "cbi %[dispoff_port], %[dispoff_bit]" "\n\t" // Kill DISPOFF
    "cbi %[bckl_port], %[bckl_bit]" "\n\t" // Kill LED backlight
  "1: "
    "ldi r24, hi8(vee_cut)" "\n\t"
    "cpi r26, lo8(vee_cut)" "\n\t"
    "cpc r27, r24" "\n\t"
    "brlo 2f" "\n\t"
    "cbi %[vee_port], %[vee_bit]" "\n\t" // KILL Vee
    "eor r26, r26" "\n\t"
    "eor r27, r27" "\n\t"
  "2: "  
    "sts %[power_cnt], r26" "\n\t"
    "sts %[power_cnt]+1, r27" "\n\t"
    "rjmp usb_ep" "\n\t" 

  "buf_undr: " // Buffer underrun 
    // Disconnect CL1 from timer
    "ldi r24, %[cl1_base]" "\n\t"
    "sts %[cl1_tccr], r24" "\n\t"         
    "cbi %[cl1_port], %[cl1_bit]" "\n\t"
    // Shift out FLM bit from common drivers
    "ldi r25, %[cl1_bv]" "\n\t"
    "lds r24, %[row_cnt]" "\n\t"    
  "3: " // loop until we shift out the row bit 
    "out %[cl1_portin], r25" ";CL1 UP \n\t"      
    "subi r24, 0x01" "\n\t"
    "out %[cl1_portin], r25" ";CL1 DOWN \n\t"      
    "brpl 3b" "\n\t" // Loop back
    // Re-connect CL1 to timer
    "ldi r24, %[cl1_set_m]" "\n\t"
    "sts %[cl1_tccr], r24" "\n\t"         
    // Reset ring buffer and row counter
    "ldi r24, 0x01" "\n\t"
    "sts %[row_cnt], r24" "\n\t"    
    "ldi r24, lo8(buf_start)" "\n\t"
    "ldi r25, hi8(buf_start)" "\n\t"    
    "sts %[buf_head], r24" "\n\t"    
    "sts %[buf_head]+1, r25" "\n\t"    
    "sts %[buf_tail], r24" "\n\t"    
    "sts %[buf_tail]+1, r25" "\n\t"
    "ldi r25, %[buf_cnt_bottom]" "\n\t"
    "rjmp usb_ep" "\n\t"

  "buf2lcd: "
    // We have data in the buffer
    // TODO: Use GPIOR2 as LCD row counter (downwards)
    "lds r24, %[row_cnt]" "\n\t"
    "subi r24, 0x01" "\n\t"
    "brne 5f" "\n\t"
    
    // Enable display
    "sbi %[dispoff_port], %[dispoff_bit]" "\n\t" 
    // Setup FLM    
    "sbi %[flm_port], %[flm_bit]" "\n\t"
    // ... and set ALT_M_OCnX to be flipped on next CL1 timer match.
    "lds r24, %[cl1_tccr]" "\n\t"     
    "ldi r30, %[cl1_toggle_m]" "\n\t"
    "eor r24, r30" "\n\t"
    "sts %[cl1_tccr], r24" "\n\t"     
    // Reset LCD row count    
    "ldi r24, 0xF0" "\n\t"
    
  "5: " // Save row counter and shift data
    "sts %[row_cnt], r24" "\n\t"
    
    // Shift data to column drivers
    "lds r26, %[buf_head]" "\n\t"
    "lds r27, %[buf_head]+1" "\n\t"     
    "ld r24, X+" "\n\t"

    // Start CL2 timer
    "eor r30, r30" "\n\t"
    "sts %[cl2_tcnt], r30\n\t"
    "ldi r30, %[cl2_start]" "\n\t"
    "sts %[cl2_tccr], r30" "\n\t"
    "nop" "\n\t" // Sync
    
    // Shift out 
    SHIFT_9X_BYTE_FROM_BUFFER()
    SHIFT_9X_BYTE_FROM_BUFFER()
    SHIFT_9X_BYTE_FROM_BUFFER()
    SHIFT_9X_BYTE_FROM_BUFFER()
    SHIFT_3X_BYTE_FROM_BUFFER()
    "out %[data_port], r24" "\n\t"
    "swap r24" "\n\t"
    //"eor r30, r30" "\n\t"
    "ldi r30, %[cl2_stop]" "\n\t"
    "out %[data_port], r24" "\n\t"
    
    // Stop CL2 timer
    "sts %[cl2_tccr], r30" "\n\t"

    // Check for end of buffer and rewind to start if needed
    "ldi r24, hi8(buf_end)" "\n\t"
    "cpi r26, lo8(buf_end)" "\n\t"
    "cpc r27, r24" "\n\t"
    "brlo 6f" "\n\t"
    "ldi r26, lo8(buf_start)" "\n\t"
    "ldi r27, hi8(buf_start)" "\n\t"
  "6: " // Save buf_head pointer
    "sts %[buf_head], r26" "\n\t"
    "sts %[buf_head]+1, r27" "\n\t" 
    
  :: [buf_cnt] "" (&buf_cnt),
     [row_cnt] "" (&row_cnt),
     [power_cnt] "" (&power_cnt),
     [buf_head] "" (&buf_head), 
     [buf_tail] "" (&buf_tail), 
     [buf_cnt_banks] "" (CLGLCD_BUFFER_BANKS),
     [buf_cnt_bottom] "" (-CLGLCD_BUFFER_BANKS),
     
     [data_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_DATA))), 
     [flm_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_FLM))),
     [flm_bit] "" (BIT(CLGLCD_FLM)),
     [cl1_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_CL1))),
     [cl1_portin] "I" (_SFR_IO_ADDR(PIN(CLGLCD_CL1))),
     [cl1_bit] "" (BIT(CLGLCD_CL1)),
     [cl1_bv] "" (_BV(BIT(CLGLCD_CL1))),
     [alt_m_portin] "I" (_SFR_IO_ADDR(PORT(CLGLCD_ALT_M))),
     [alt_m_bit] "" (BIT(CLGLCD_ALT_M)),
     [dispoff_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_DISPOFF))),
     [dispoff_bit] "" (BIT(CLGLCD_DISPOFF)),
     [bckl_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_BCKL_CTRL))),
     [bckl_bit] "" (BIT(CLGLCD_BCKL_CTRL)),
     [vee_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_VEE_CTRL))),
     [vee_bit] "" (BIT(CLGLCD_VEE_CTRL)),
     
     // Timer registers and values
     [cl1_tccr] "" (&CL1_TIMER_CR),     
     [cl1_base] "" (CL1_BASE),
     [cl1_set_m] "" (CL1_BASE | CL1_SET | CL1_SET_M),
     [cl1_toggle_m] "" (CL1_TOGGLE_M),
     [cl2_tcnt] "" (TM_CNT_L(CLGLCD_CL2_OCnX)),
     [cl2_tccr] "" (&TM_CR(CLGLCD_CL2_OCnX, B)),
     [cl2_start] "" (CL2_START),
     [cl2_stop] "" (CL2_STOP)
 );

 __asm__ (
  "usb_ep: "     
    // USB endpoint management
    // If there is space in the buffer
    "cpi r25, %[buf_cnt_banks]-3" "\n\t"
    "brlt 1f" "\n\t"
    "rjmp leave" "\n\t"
  "1: " // We have room in buffer
    // Load Z with usb controller base address
    "eor r31, r31" "\n\t"
    "ldi r30, %[usb_base]" "\n\t"
    // Save EP to stack
    // It takes 4 ticks (plus 4 to restore) but it 
    // enables us to run on top of other USB code 
    // (i.e. USBCore, with interrupts enabled...
    "ldd r24, Z+%[uenum_ofs]" "\n\t"
    "push r24" "\n\t"
    // Set EP
    "lds r24, %[ep_num]" "\n\t" 
    "std Z+%[uenum_ofs], r24" "\n\t"     
    // Check if we have packet in usb fifo
    "ldd r24, Z+%[ueintx_ofs]" "\n\t"
    "sbrs r24, %[rxouti_bit]" "\n\t" 
    "rjmp restore_ep" "\n\t"

    // Check number of bytes in USB packet
    "ldd r24, Z+%[uebclx_ofs]" "\n\t"
    "cpi r24, 0x3D" "\n\t"
    "brlo 2f" "\n\t"

    // Load packet number from fifo
    "ldd r24, Z+%[uedatx_ofs]\n\t"

    // Are we at the bottom >
    "cpi r25, %[buf_cnt_bottom]+1" "\n\t"
    "brge usb2buf" "\n\t"
    // We at the bottom, wait for first packet 
    "cpi r24, 0" "\n\t"
    "breq usb2buf" "\n\t"
  "2: " // Do not load data, just ack the packet
    "rjmp ack_fifo" "\n\t"
  
  "usb2buf: " 
    "lds r26, %[buf_tail]" "\n\t"
    "lds r27, %[buf_tail]+1" "\n\t"
    //"out %[data_port], r27" "\n\t" 
    MOVE_10x_USB_BYTE_TO_BUFFER()
    MOVE_10x_USB_BYTE_TO_BUFFER()
    MOVE_10x_USB_BYTE_TO_BUFFER()
    MOVE_10x_USB_BYTE_TO_BUFFER()
    MOVE_10x_USB_BYTE_TO_BUFFER()
    MOVE_10x_USB_BYTE_TO_BUFFER()
    "ldi r24, hi8(buf_end)" "\n\t"
    "cpi r26, lo8(buf_end)" "\n\t"
    "cpc r27, r24" "\n\t"
    "brlo 3f" "\n\t"
    "ldi r26, lo8(buf_start)" "\n\t"
    "ldi r27, hi8(buf_start)" "\n\t"
  "3: " // Save tail and increment buffer counter
    "sts %[buf_tail], r26" "\n\t"
    "sts %[buf_tail]+1, r27" "\n\t" 
    // Add 3 banks (60 bytes) to counter
    "subi r25, 0xFD" "\n\t" 
    // If we equal zero...
    "brne ack_fifo" "\n\t"
    // we were buffering data and need to enable LCD output. 
    // Change buf_cnt to positive max banks,
    "ldi r25, %[buf_cnt_banks]" "\n\t"
    // and enable Vee power and backlight,
    "sbi %[bckl_port], %[bckl_bit]" "\n\t"
    "sbi %[vee_port], %[vee_bit]" "\n\t"

  "ack_fifo:"
    "ldi r24, %[ack_bits]" "\n\t" // ACK BITS
    "std Z+%[ueintx_ofs], r24" "\n\t"
    
  "restore_ep:"
    "pop r24" "\n\t"
    "std Z+%[uenum_ofs], r24" "\n\t"     
    
  :: [buf_cnt] "" (&buf_cnt),
     [buf_tail] "" (&buf_tail), 
     [buf_cnt_banks] "" (CLGLCD_BUFFER_BANKS),
     [buf_cnt_bottom] "" (-CLGLCD_BUFFER_BANKS),

     [bckl_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_BCKL_CTRL))),
     [bckl_bit] "" (BIT(CLGLCD_BCKL_CTRL)),
     [vee_port] "I" (_SFR_IO_ADDR(PORT(CLGLCD_VEE_CTRL))),
     [vee_bit] "" (BIT(CLGLCD_VEE_CTRL)),
     
     [ep_num] "" (&lcd_ep),       
     [usb_base] "" (&UDCON),
     [uenum_ofs] "" (&UENUM-&UDCON),
     [uesta0x_ofs] "" (&UESTA0X-&UDCON),
     [ueintx_ofs] "" (&UEINTX-&UDCON),
     [uebclx_ofs] "" (&UEBCLX-&UDCON),
     [uedatx_ofs] "" (&UEDATX-&UDCON),
     [rxouti_bit] "" (RXOUTI),
     [ack_bits] "" (0x6B) // FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
 );

 __asm__ (
  "leave:"    
    "sts %[buf_cnt], r25" "\n\t"
    "pop r31" "\n\t"
    "pop r30" "\n\t"
    "pop r27" "\n\t"
    "pop r26" "\n\t"
    "pop r25" "\n\t"    
    "pop r24" "\n\t"
    "out __SREG__, r24" "\n\t"
    "pop r24" "\n\t"
    "reti" "\n\t"
    :: [buf_cnt] "" (&buf_cnt)
  );    
}

// Reads 'raw' touchpanel resitances, leaving positional math
// to the host application
bool read_touchpanel() {
  int16_t z1, z2;

  // To avoild disabling interrputs, use z1
  // value as atomic 8bit indicatior. 
  // If z1 < 0, measurement is in progress and
  // any other value in the report is invalid
  // Assume ADC is 10bit, but 7bit resolutin for 
  // the presure level z1 and z2 shoud be sufficient

  // Setup grid to read presure 
  OUTPUT_PIN(CLGLCD_TP_XR); // xR -> GND
  CLEAR(CLGLCD_TP_XR);  
  OUTPUT_PIN(CLGLCD_TP_YU); // yU -> Vcc
  SET(CLGLCD_TP_YU);
  INPUT_PIN(CLGLCD_TP_XL);  // xL -> Hi Z
  CLEAR(CLGLCD_TP_XL);
  INPUT_PIN(CLGLCD_TP_YL);  // yL -> Hi Z
  CLEAR(CLGLCD_TP_YL);
  
  // ADC is slow (default 125KHz) and MSB 
  // is read first. That should give it enough 
  // time for grid to settle without delay

  // Read voltage on XL
  z1 = analogRead(CLGLCD_TP_XL_AN) >> 3;
  
  // No voltage on XL in this setup, means no touch
  if (z1 < 1) {
    USB.tp_report.z1 = -1; 
    USB.tp_report.z2 = 127;    
    USB.tp_report.x = -1;
    USB.tp_report.y = -1;
    // Increase counter to indicate new measurement
    USB.tp_report.cnt++;
    USB.tp_report.z1 = 0; 
    return false;
  }
  
  // Too high voltage on Z1, bad reading
  if (z1 > CLGLCD_TP_CAL_Z1_MAX) return false;

  // Read voltage on YL
  z2 = analogRead(CLGLCD_TP_YL_AN) >> 3;
  // yL should be pulled high if touchpannel is connected
  // if there is low value on yL, likely touchpannel is not connected
  if (z2 < CLGLCD_TP_CAL_Z2_MIN) return false;
  
  // Mark any reading of tp_report (i.e. by ISR) as invalid
  USB.tp_report.z1 = -1;

  // Setup grid for Y measurement  
  INPUT_PIN(CLGLCD_TP_XR);
  CLEAR(CLGLCD_TP_YU);
  OUTPUT_PIN(CLGLCD_TP_YL); 
  SET(CLGLCD_TP_YL);;
 
  // Read voltage on XL
  USB.tp_report.y = analogRead(CLGLCD_TP_XL_AN);

  // Setup grid for X measurement
  INPUT_PIN(CLGLCD_TP_YL);
  CLEAR(CLGLCD_TP_YL); 
  INPUT_PIN(CLGLCD_TP_YU);
  OUTPUT_PIN(CLGLCD_TP_XL);
  OUTPUT_PIN(CLGLCD_TP_XR); 
  SET(CLGLCD_TP_XR);  

  // Read voltage on YL
  USB.tp_report.x = analogRead(CLGLCD_TP_YL_AN);  

  // Increase counter to indicate new measurement
  USB.tp_report.cnt++;
  USB.tp_report.z2 = z2;
  USB.tp_report.z1 = z1;

  return true;
}

void loop() { 
  read_touchpanel();

  // Call the USB interrupt handlers with interrputs enabled!
  #ifdef DISABLE_USB_CORE_IRQ
    // Check if we have any device level interrupt flag
    if (UDINT != 0) { 
      asm volatile ("CALL __vector_10\n\t"); // USB_GEN_vect(); 
      // Carefull here, ISR may re-enable interrputs for suspend/wakeup... 
      // TODO: Well, disable those again
    }
    // USB_COM_vect checks inside if it received setup packet
    // on EP0, and clears the flag itself...
    // Seems to be safe to call without us double checking...
    asm volatile ("CALL __vector_11\n\t"); // USB_COM_vect();
  #endif
}
  
