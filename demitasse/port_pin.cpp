#include "port_pin.h"

const PortPin::PinMap PortPin::pinMap[] =
{ // DIO
  { 0xff, 0xff },

#ifndef TESTBUILD
  { IOPORT_F, 2 },
  { IOPORT_F, 3 },
  { IOPORT_D, 8 },
  { IOPORT_D, 0 },
  { IOPORT_F, 1 },
  { IOPORT_D, 1 },
  { IOPORT_D, 2 },
  { IOPORT_D, 9 },
  { IOPORT_D, 10 },
  { IOPORT_D, 3 },
  { IOPORT_G, 9 }, // RD4, RG9, alternate
  { IOPORT_G, 7 }, // RG8, alternate
  { IOPORT_G, 8 }, // RG7, alternate
  { IOPORT_G, 6 },

  // AIO
  { IOPORT_B, 2 },
  { IOPORT_B, 4 },
  { IOPORT_B, 8 },
  { IOPORT_B, 10 },
  { IOPORT_B, 12 },  // RG3, alternate
  { IOPORT_B, 14 },  // RG2, alternate
#endif
};
