#include "buffered_serial.h"

BufferedSerial gSerial;

#ifndef TESTBUILD

extern "C"
{
  
  void __ISR(_UART1_VECTOR, ipl2) IntUart1Handler(void)
  {
    gSerial.HandleUartInterrupt();
  }

} // extern "C"

#endif

