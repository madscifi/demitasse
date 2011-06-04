#ifndef PORT_PIN_H_
#define PORT_PIN_H_

#ifndef TESTBUILD
#include <plib.h>
#endif

#include "processor.h"

struct PortPin
{
  enum BoardPinType 
  { 
    IONULL, 
    DIO0, 
    DIO1, 
    DIO2, 
    DIO3, 
    DIO4, 
    DIO5, 
    DIO6, 
    DIO7, 
    DIO8, 
    DIO9, 
    DIO10, 
    DIO11, 
    DIO12, 
    DIO13, 
    AIO0, 
    AIO1, 
    AIO2, 
    AIO3, 
    AIO4, 
    AIO5 
  };

  struct PinMap
  {
    uint8_t portId;
    uint8_t pinNumber;
  }; 
	 
  static const PinMap pinMap[];

  uint32_t boardPinNumber;
  bool invert;

  IoPortId cachedPort;
  uint32_t cachedMask;

  IoPortId GetPortId() const
  {
#ifndef TESTBUILD
    return static_cast<IoPortId>(pinMap[boardPinNumber].portId);
#else
    return 0;
#endif
  }

  uint32_t GetPinMask() const
  {
#ifndef TESTBUILD
    return 1u << pinMap[boardPinNumber].pinNumber;
#else
    return 0;
#endif
  }

  void SetAsOutput()
  {
    if( boardPinNumber )
    {
      PORTSetPinsDigitalOut( cachedPort, cachedMask );
    }
  }

  void SetAsInput()
  {
    if( boardPinNumber )
    {
      PORTSetPinsDigitalIn( cachedPort, cachedMask );
    }
  }

  void SetTo( bool state )
  {
    if( boardPinNumber )
    {
      if( state )
      {
        Set();
      }
      else
      {
        Clear();
      }
    }
  }

  void Set()
  {
    if( boardPinNumber )
    {
      if( invert )
      {
        PORTClearBits( cachedPort, cachedMask );
      }
      else
      {
        PORTSetBits( cachedPort, cachedMask );
      }
    }
  }

  void Clear()
  {
    if( boardPinNumber )
    {
      if( !invert )
      {
        PORTClearBits( cachedPort, cachedMask );
      }
      else
      {
        PORTSetBits( cachedPort, cachedMask );
      }
    }
  }

  void Init()
  {
    if( boardPinNumber )
    {
      cachedPort = GetPortId();
      cachedMask = GetPinMask();
      SetTo( invert );
      PORTSetPinsDigitalOut( cachedPort, cachedMask );
    }
  }
};

#endif
