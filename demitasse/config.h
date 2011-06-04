#ifndef CONFIG_H_
#define CONFIG_H_

#include "processor.h"

// units expressed as parts per meter in which positions are
// represented. At present only the value of 1000000 is supported,
// and in the future this configuration, if it does not go away,
// will probably be changed to an enumeration as the size of 
// some variables need to change to support larger values.
const uint32_t POSITION_UNITS_PARTS_PER_METER = 1000000;

// If true, uses approximate distance calculations. This
// is faster and uses about 3k less flash, but distance
// calculations can be off by up to ? percent, which will 
// effect the feed rates and extruder speed. At present 
// there is no evidence to support the idea that the more 
// accurate setting makes a detectable difference in the 
// quality of a print.
const bool APRROXIMATE_DISTANCE_CALCULATION = true;

enum E_AXIS_BEHAVIOR_TYPE 
{ 
  E_AXIS_ABSOLUTE,                 // absolute E mode -- has not been tested
  E_AXIS_RELATIVE_TRUNCATE,        // relative E mode -- small errors accumulate
  E_AXIS_RELATIVE_ERROR_ACCUMULATE // relative E mode with tracking/adjustments of errors
                                   // not really useful unless the position resolution is 
                                   // at least several times smaller than a single step
};

const E_AXIS_BEHAVIOR_TYPE E_AXIS_BEHAVIOR = E_AXIS_RELATIVE_ERROR_ACCUMULATE;

/***************************************************************************\
*                                                                           *
* 1. MECHANICAL/HARDWARE                                                    *
*                                                                           *
\***************************************************************************/

// calculate these values appropriate for your machine
// for threaded rods, this is (steps motor per turn) / (pitch of the thread)
// for belts, this is (steps per motor turn) / (number of gear teeth) / (belt module)
// half-stepping doubles the number, quarter stepping requires * 4, etc.
const int32_t MICROSTEPPING_X = 16;
const int32_t MICROSTEPPING_Y = 16;
const int32_t MICROSTEPPING_Z = 2;
const int32_t MICROSTEPPING_E = 16;

// 200 / (18 * 2.032)
const int32_t STEPS_PER_M_X = (   5.468 * 1000 /*mm/m*/ * MICROSTEPPING_X);
// 200 / (15 * 2.032)
const int32_t STEPS_PER_M_Y = (   6.561 * 1000 /*mm/m*/ * MICROSTEPPING_Y);

const int32_t STEPS_PER_M_Z = ( 416.699 * 1000 /*mm/m*/ * MICROSTEPPING_Z);

// http://blog.arcol.hu/?p=157 may help with this next one
const int32_t STEPS_PER_M_E = (  38.500 * 1000 /*mm/m*/ * MICROSTEPPING_E);

#define PID_SCALE 1024

/*
	Values depending on the capabilities of your stepper motors and other mechanics.
		All numbers are integers, no decimals allowed.

		Units are mm/min
*/

// used for G0 rapid moves and as a cap for all other feedrates

// In MM/M
const int32_t MAXIMUM_FEEDRATE_X_MM_M = 2000;
const int32_t MAXIMUM_FEEDRATE_Y_MM_M = 2000;
const int32_t MAXIMUM_FEEDRATE_Z_MM_M = 30;
const int32_t MAXIMUM_FEEDRATE_E_MM_M = 400;

// used when searching endstops and as default feedrate
// In MM/M
const int32_t SEARCH_FEEDRATE_X	= 50;
const int32_t SEARCH_FEEDRATE_Y = 50;
const int32_t SEARCH_FEEDRATE_Z = 1;
const int32_t SEARCH_FEEDRATE_E = 50;

const bool X_INVERT_DIR = true;
const bool Y_INVERT_DIR = true;
const bool Z_INVERT_DIR = true;
const bool E_INVERT_DIR = true;

#endif // CONFIG_H_
