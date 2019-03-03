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

#ifndef CLGLCD_H
#define CLGLCD_H

#include <Arduino.h>

#ifndef CLGLCD_CONFIG
#include "clglcd_config.h"
#endif
#ifndef CLGLCD_FONT_LINES
#include "clglcd_font.h"
#endif

// Screen SRAM buffer: screen[y][x]
extern uint8_t volatile screen[CLGLCD_Y_LINES][40]; 

// SRAM font buffer: soft_font[character_line][character_offset]
#if CLGLCD_SOFT_CHARS > 0
extern uint8_t volatile soft_font[CLGLCD_FONT_LINES][CLGLCD_SOFT_CHARS] __attribute__((aligned(CLGLCD_SOFT_CHARS))); 
#endif

// Setup LCD output pins and clear screen
void CLGLCD_init();

// Turn on the LCD and start driving interrupt
void CLGLCD_on();

// Turn off the LCD and stop driving interrupt
void CLGLCD_off();

// Clear the screen, fill the screen buffer with zeroes
void CLGLCD_clear_screen();

// Check if FLM signal is up, usefull when disabling interrputs
bool CLGLCD_FLM_is_up();


#endif // CLGLCD_H
