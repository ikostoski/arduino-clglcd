/*
 * PluggableUSB/WCID Implementation
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

#include "PluggableUSB.h"

struct CLGLCD_tp_report {
  union {
    uint8_t buf[8];
    struct {
      int8_t z1;
      int8_t z2;
      int16_t x;
      int16_t y;
      int16_t cnt;
    };
  };
};

class CLGLCD_USB_ : public PluggableUSBModule {
  public:
    CLGLCD_USB_(void);    
    CLGLCD_tp_report tp_report;
    uint8_t getEP();
  protected:
    int getInterface(uint8_t* interfaceCount);
    int getDescriptor(USBSetup& setup);
    bool setup(USBSetup& setup);
    uint8_t getShortName(char *name);
  private:
    uint8_t epType[1];
};
