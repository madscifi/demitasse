#include "step_generator.h"
#include "port_pin.h"
#include "gcode_parse.h"

PortPin enableSteppersPin = { PortPin::DIO11, 0 };

int main(void)
{
  unsigned int pbClock = SYSTEMConfigPerformance(F_CPU);
 
  debugPin.Init(); 
  enableSteppersPin.Init();
  
  //OpenCoreTimer(CORE_TICK_RATE);
  // set up the core timer interrupt with a prioirty of 2 and zero sub-priority
  //mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));
  
  DDPCONbits.JTAGEN	=	0;

  gSerial.Init( 115200, pbClock );
  StepGenerator::Init();
  
  MotionPlanner::Init();
  
  INTEnableSystemMultiVectoredInt();

  enableSteppersPin.Set();
  
  gSerial.WriteStringBlocking( "start\n" );
  
  while(1)
  {    
    if( !StepGenerator::IsFull() )
    {
      int c = gSerial.GetCharNoBlocking();
      if( c >= 0 )
      {
        //debugPin.Set();        
        gcode_parse_char(c);
        //debugPin.Clear();        
      }
    }
  }
}

