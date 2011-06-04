#include "step_generator.h"
#include "config.h"

#ifdef TESTBUILD
#include "gcode_parse.h"
#endif

PortPin debugPin = { PortPin::AIO0, 0 };

unsigned int StepGenerator::live_axis_count = 0;

// x step pin DIO3
// y_step pin DIO5
// z_step pin DIO6
// e_step pin DIO9

PortPin StepGenerator::dirPins[] = 
{ 
  { PortPin::DIO2, X_INVERT_DIR }, // x dir pin
  { PortPin::DIO4, Y_INVERT_DIR }, // y dir pin
  { PortPin::DIO7, Z_INVERT_DIR }, // z dir pin
  { PortPin::DIO8, E_INVERT_DIR }  // e dir pin
};

StepGenerator::RingBufferType StepGenerator::stepper_move_buffer;

#ifndef TESTBUILD

extern "C"
{

void __ISR(_OUTPUT_COMPARE_1_VECTOR, ipl5) OC1_IntlHandler(void)
{
  StepGenerator::Handle_OC1();
}

void __ISR(_OUTPUT_COMPARE_2_VECTOR, ipl5) OC2_IntlHandler(void)
{
  StepGenerator::Handle_OC2();
}

void __ISR(_OUTPUT_COMPARE_3_VECTOR, ipl5) OC3_IntlHandler(void)
{
  StepGenerator::Handle_OC3();
}
void __ISR(_OUTPUT_COMPARE_4_VECTOR, ipl5) OC4_IntlHandler(void)
{
  StepGenerator::Handle_OC4();
}

} // extern "C"

#endif


