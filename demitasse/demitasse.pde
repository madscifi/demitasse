#include "step_generator.h"
#include "port_pin.h"
#include "gcode_parse.h"

PortPin enableSteppersPin = { PortPin::DIO11, 0 };

// WORKAROUND for mpide 0023-windows-20130715
// wiring.c defines the variable:
//   extern const uint32_t __attribute__((section(".mpide_version"))) _verMPIDE_Stub = MPIDEVER;    // assigns the build number in the header section in the image
// The linker script includes the .mpide_version section in the .header_info section as:
//   KEEP(*(.mpide_version))
// The linker script verifies that the length of the .header_info section is the expected length.
// If the .mpide_version section is not found by the linker then the length of the .header_info 
// section will be wrong and the error "MPIDE Version not specfied correctly" will be generated 
// by the linker. 
//
// It is not clear if KEEP is failing to work as desired or this code is strictly at fault. 
// This problem occurs with this code because this code replaces the startup code that is normally 
// found in wiring.c. Everything used to build correctly before 0023-windows-20130715. The immediate 
// workaround is to simply define the variable here. Referencing something in wiring.c is another 
// solution to the problem, but that increases the size of the code since it causes more unnecessary 
// code to be included in the executable.
//
// Also note that I don't have a testbed on which to run the code setup, so I've only verified
// that this fixes the build problem - I don't know if the result actually runs...
#ifdef MPIDEVER
extern const uint32_t __attribute__((section(".mpide_version"))) _verMPIDE_Stub = MPIDEVER;    // assigns the build number in the header section in the image
#endif
// END WORKAROUND for mpide 0023-windows-20130715

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

