// CLGLCD Custom Exception

#ifndef CLGLCD_EXC_H
#define CLGLCD_EXC_H

#include <exception>
#include <stdexcept>

// Major error, will terminate program
class CLGLCD_error: public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

// If not device is present or stopped responding
class CLGLCD_device_error: public CLGLCD_error {
  public:
    using CLGLCD_error::CLGLCD_error;
};

// Nothing serious, sometimes transfers do fail
class CLGLCD_transfer_error: public CLGLCD_device_error {
  public:
    using CLGLCD_device_error::CLGLCD_device_error;
};

#endif // CLGLCD_EXC_H
