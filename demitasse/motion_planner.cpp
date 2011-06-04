#include "motion_planner.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "gcode_parse.h"
#include "step_generator.h"


#define	max(a,b)	(((a) > (b)) ? (a) : (b))

TARGET MotionPlanner::startpoint;
TARGET MotionPlanner::startpoint_steps;
MotionPlanner::e_error_states  MotionPlanner::e_error_state = NORMAL;
int32_t MotionPlanner::e_error = 0;

/*
	utility functions
*/

// courtesy of http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
/*! linear approximation 2d distance formula
	\param dx distance in X plane
	\param dy distance in Y plane
	\return 3-part linear approximation of \f$\sqrt{\Delta x^2 + \Delta y^2}\f$

	see http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
*/
uint32_t approx_distance( uint32_t dx, uint32_t dy )
{
	uint32_t min, max, approx;

	if ( dx < dy )
	{
		min = dx;
		max = dy;
	} else {
		min = dy;
		max = dx;
	}

	approx = ( max * 1007 ) + ( min * 441 );
	if ( max < ( min << 4 ))
		approx -= ( max * 40 );

	// add 512 for proper rounding
	return (( approx + 512 ) >> 10 );
}

// courtesy of http://www.oroboro.com/rafael/docserv.php/index/programming/article/distance
/*! linear approximation 3d distance formula
	\param dx distance in X plane
	\param dy distance in Y plane
	\param dz distance in Z plane
	\return 3-part linear approximation of \f$\sqrt{\Delta x^2 + \Delta y^2 + \Delta z^2}\f$

	see http://www.oroboro.com/rafael/docserv.php/index/programming/article/distance
*/
uint32_t approx_distance_3( uint32_t dx, uint32_t dy, uint32_t dz )
{
	uint32_t min, med, max, approx;

	if ( dx < dy )
	{
		min = dy;
		med = dx;
	} else {
		min = dx;
		med = dy;
	}

	if ( dz < min )
	{
		max = med;
		med = min;
		min = dz;
	} else if ( dz < med ) {
		max = med;
		med = dz;
	} else {
		max = dz;
	}

	approx = ( max * 860 ) + ( med * 851 ) + ( min * 520 );
	if ( max < ( med << 1 )) approx -= ( max * 294 );
	if ( max < ( min << 2 )) approx -= ( max * 113 );
	if ( med < ( min << 2 )) approx -= ( med *  40 );

	// add 512 for proper rounding
	return (( approx + 512 ) >> 10 );
}

/*!
	integer square root algorithm
	\param a find square root of this number
	\return sqrt(a - 1) < returnvalue <= sqrt(a)

	see http://www.embedded-systems.com/98/9802fe2.htm
*/
// courtesy of http://www.embedded-systems.com/98/9802fe2.htm
uint16_t int_sqrt(uint32_t a) {
	uint32_t rem = 0;
	uint32_t root = 0;
	uint16_t i;

	for (i = 0; i < 16; i++) {
		root <<= 1;
		rem = ((rem << 2) + (a >> 30));
		a <<= 2;
		root++;
		if (root <= rem) {
			rem -= root;
			root++;
		}
		else
			root--;
	}
	return (uint16_t) ((root >> 1) & 0xFFFFL);
}

uint32_t int_distance( uint32_t a, uint32_t b )
{
  if( APRROXIMATE_DISTANCE_CALCULATION )
  {
    return approx_distance( a, b );
  }
  return sqrt( (double)a*a + (double)b*b ) + 0.5;
}
uint32_t int_distance_3( uint32_t a, uint32_t b, uint32_t c )
{
  if( APRROXIMATE_DISTANCE_CALCULATION )
  {
    return approx_distance_3( a, b, c );
  }
  return sqrt( (double)a*a + (double)b*b + (double)c*c ) + 0.5;
}

/*! Inititalise DDA movement structures
*/
void MotionPlanner::Init() 
{
	// set up default feedrate
	startpoint.X = /*current_position.X =*/ next_target.target.X = startpoint_steps.X =
	startpoint.Y = /*current_position.Y =*/ next_target.target.Y = startpoint_steps.Y =
	startpoint.Z = /*current_position.Z =*/ next_target.target.Z = startpoint_steps.Z = 0;

	//current_position.F = startpoint.F = next_target.target.F = SEARCH_FEEDRATE_Z;
}

void MotionPlanner::ScheduleMovement( TARGET* target )
{
  //debugPin.Set();
  
  #if DEBUG_STEP_GENERATION
  sersendf_P( "tx=%d ty=%d tz=%d te=%d ", target->X, target->Y, target->Z, target->E );
  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif
  #endif

  if( !target ) return;
  
  // _pu stands for position units, as specified by POSITION_UNITS_PARTS_PER_METER
  int32_t x_delta_pu = target->X - startpoint.X;
  int32_t y_delta_pu = target->Y - startpoint.Y;
  int32_t z_delta_pu = target->Z - startpoint.Z;
  int32_t e_delta_pu = target->E - startpoint.E;
  int32_t e_request_delta_pu = e_delta_pu;
  
  #if DEBUG_STEP_GENERATION
  sersendf_P( "dxu=%d dyu=%d dzu=%d deu=%d ", x_delta_pu, y_delta_pu, z_delta_pu, e_delta_pu );
  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif
  #endif

  int32_t use_e_error = 0;
  if( E_AXIS_BEHAVIOR == E_AXIS_RELATIVE_ERROR_ACCUMULATE )
  {
    if( e_delta_pu > 0 )
    {
      use_e_error = e_error;
    }
    e_delta_pu += use_e_error;
  }

  #if DEBUG_STEP_GENERATION
  sersendf_P( "deu(aea)=%d ", e_delta_pu );
  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif
  #endif
  
  int32_t pu_rounding = POSITION_UNITS_PARTS_PER_METER / 2;

  int32_t target_x_steps = ((int64_t)target->X * (int64_t) STEPS_PER_M_X + ((target->X>=0)?pu_rounding:-pu_rounding)) / POSITION_UNITS_PARTS_PER_METER;
  int32_t target_y_steps = ((int64_t)target->Y * (int64_t) STEPS_PER_M_Y + ((target->Y>=0)?pu_rounding:-pu_rounding)) / POSITION_UNITS_PARTS_PER_METER;
  int32_t target_z_steps = ((int64_t)target->Z * (int64_t) STEPS_PER_M_Z + ((target->Z>=0)?pu_rounding:-pu_rounding)) / POSITION_UNITS_PARTS_PER_METER;
  int32_t target_e_steps = (((int64_t)target->E+use_e_error) * (int64_t) STEPS_PER_M_E + ((target->E>=0)?pu_rounding:-pu_rounding)) / POSITION_UNITS_PARTS_PER_METER;
  
  int32_t x_magnitude_steps = abs(target_x_steps - startpoint_steps.X);
  int32_t y_magnitude_steps = abs(target_y_steps - startpoint_steps.Y);
  int32_t z_magnitude_steps = abs(target_z_steps - startpoint_steps.Z);
  int32_t e_magnitude_steps = abs(target_e_steps - startpoint_steps.E);
  

  #if DEBUG_STEP_GENERATION
  sersendf_P( "mxs=%d mys=%d mzs=%d mes=%d ", x_magnitude_steps, y_magnitude_steps, z_magnitude_steps, e_magnitude_steps );
  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif
  #endif

  int x_magnitude_steps_pu = ((int64_t)x_magnitude_steps * POSITION_UNITS_PARTS_PER_METER + (STEPS_PER_M_X/2)) / (int64_t) STEPS_PER_M_X;
  int y_magnitude_steps_pu = ((int64_t)y_magnitude_steps * POSITION_UNITS_PARTS_PER_METER + (STEPS_PER_M_Y/2)) / (int64_t) STEPS_PER_M_Y;
  int z_magnitude_steps_pu = ((int64_t)z_magnitude_steps * POSITION_UNITS_PARTS_PER_METER + (STEPS_PER_M_Z/2)) / (int64_t) STEPS_PER_M_Z;
  int e_magnitude_steps_pu = ((int64_t)e_magnitude_steps * POSITION_UNITS_PARTS_PER_METER + (STEPS_PER_M_E/2)) / (int64_t) STEPS_PER_M_E;

  #if DEBUG_STEP_GENERATION
  sersendf_P( "mxsu=%d mysu=%d mzsu=%d mesu=%d ", x_magnitude_steps_pu, y_magnitude_steps_pu, z_magnitude_steps_pu, e_magnitude_steps_pu );
  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif
  #endif

  uint32_t max_steps_on_any_planer_axis = max( x_magnitude_steps, max( y_magnitude_steps, z_magnitude_steps ) );
  
  uint32_t traval_magnitude_steps_pu = 0;
  uint32_t max_travel_on_axis_steps = 0;

  if( max_steps_on_any_planer_axis == 0 )
  {
    if( e_magnitude_steps == 0 || (x_delta_pu != 0 || y_delta_pu != 0 || z_delta_pu != 0 )) 
    {
      startpoint = *target;
      
      #if DEBUG_STEP_GENERATION
      sersendf_P( "eerr=%d ", e_error );
      #if TESTBUILD
      sersendf_P( "\n  " );
      #endif
      #endif
  
      if( E_AXIS_BEHAVIOR != E_AXIS_ABSOLUTE )
      {
        startpoint.E = 0;
      }
      return;
    }

    traval_magnitude_steps_pu = e_magnitude_steps_pu;
    max_travel_on_axis_steps = e_magnitude_steps;
  }
  else
  {
    if( z_magnitude_steps_pu == 0 ) traval_magnitude_steps_pu = int_distance(x_magnitude_steps_pu, y_magnitude_steps_pu);
    else if( x_magnitude_steps_pu == 0 && y_magnitude_steps_pu == 0 ) traval_magnitude_steps_pu = z_magnitude_steps_pu;
    else traval_magnitude_steps_pu = int_distance_3( x_magnitude_steps_pu, y_magnitude_steps_pu, z_magnitude_steps_pu);
    max_travel_on_axis_steps = max_steps_on_any_planer_axis;
  }

  int32_t x_dir = x_delta_pu >= 0;
  int32_t y_dir = y_delta_pu >= 0;
  int32_t z_dir = z_delta_pu >= 0;
  int32_t e_dir = e_delta_pu >= 0;
  

#if DEBUG_STEP_GENERATION
  sersendf_P( "msoaa=%d ", max_steps_on_any_planer_axis );
#endif

#if DEBUG_STEP_GENERATION
  sersendf_P( "dum=%d ", traval_magnitude_steps_pu );
#endif

  // target->F is mm/m
  // d (um) / ( F mm/min * 1000 ) um/min = min
  // d (um) / ( F mm/min * 1000 ) um/min  * 60 sec/min * 1000000 us/sec * 5 ticks/us
  // ( d um * 60 sec/min * 1000000 us/sec * 5 ticks/us ) / ( F mm/min * 1000 um/mm )
  //  um* s/m * us/s * ticks/s / um/min
  //  um * s/m * us/s * t/s * min/um
  //  um * min/um * s/m * us/s * t/s = ticks

  // a tick is .2 us
  unsigned int move_duration_at_requested_rate_ticks = ((int64_t)traval_magnitude_steps_pu * 60 * 1000000 * 5) / ((int64_t)target->F * POSITION_UNITS_PARTS_PER_METER/1000);

  // compute durations assuming the max feedrate for each axis
  unsigned int move_duration_at_max_x_rate_ticks = ((int64_t)x_magnitude_steps_pu * 60 * 1000000 * 5) / ((int64_t)MAXIMUM_FEEDRATE_X_MM_M * POSITION_UNITS_PARTS_PER_METER/1000);
  unsigned int move_duration_at_max_y_rate_ticks = ((int64_t)y_magnitude_steps_pu * 60 * 1000000 * 5) / ((int64_t)MAXIMUM_FEEDRATE_Y_MM_M * POSITION_UNITS_PARTS_PER_METER/1000);
  unsigned int move_duration_at_max_z_rate_ticks = ((int64_t)z_magnitude_steps_pu * 60 * 1000000 * 5) / ((int64_t)MAXIMUM_FEEDRATE_Z_MM_M * POSITION_UNITS_PARTS_PER_METER/1000);
  unsigned int move_duration_at_max_e_rate_ticks = ((int64_t)e_magnitude_steps_pu * 60 * 1000000 * 5) / ((int64_t)MAXIMUM_FEEDRATE_E_MM_M * POSITION_UNITS_PARTS_PER_METER/1000);

  // the longest interval is the lower bound for what is an acceptable speed
  unsigned int move_duration_limit_ticks 
	  = max( move_duration_at_max_x_rate_ticks, 
			 max( move_duration_at_max_y_rate_ticks,
				  max( move_duration_at_max_z_rate_ticks,
			           move_duration_at_max_e_rate_ticks )));

  // use the requested duration unless it is smaller than the lower bound limit duration
  unsigned int use_move_duration_ticks = max( move_duration_at_requested_rate_ticks, move_duration_limit_ticks );
  
  // Additional limit in order to make certain the step rate is not greater than what
  // the pic can generate reliably.
  unsigned int step_interval_ticks = use_move_duration_ticks / max_travel_on_axis_steps;
  if( step_interval_ticks < 100 )
  {
    use_move_duration_ticks = 100 * max_travel_on_axis_steps;
  }

  #if DEBUG_STEP_GENERATION
  if( use_move_duration_ticks != move_duration_at_requested_rate_ticks )
  {
    sersendf_P( "mdarrt=%d ", move_duration_at_requested_rate_ticks );
  }
  sersendf_P( "umdt=%d ", use_move_duration_ticks );
  #endif

  StepGenerator::StepperMoves* moves = 0;
  do
  {
    moves = StepGenerator::ReserveMove();
  } while ( moves == 0 );
  
  memset( moves, 0, sizeof( *moves ) );
  if( x_magnitude_steps )
  {
    unsigned int step_interval_ticks = use_move_duration_ticks / x_magnitude_steps;
    unsigned int step_interval_remainder = use_move_duration_ticks % x_magnitude_steps;
    
    moves->moveCommands[X_STEPPER_INDEX].state = 1;
    moves->moveCommands[X_STEPPER_INDEX].initial_delay = step_interval_ticks;
    if( step_interval_remainder )
    {
      moves->moveCommands[X_STEPPER_INDEX].fast_delay = step_interval_ticks;
      moves->moveCommands[X_STEPPER_INDEX].fast_delay_count = x_magnitude_steps - step_interval_remainder - 1;
      moves->moveCommands[X_STEPPER_INDEX].slow_delay = step_interval_ticks + 1;
      moves->moveCommands[X_STEPPER_INDEX].slow_delay_count = step_interval_remainder;
    }
    else
    {
      moves->moveCommands[X_STEPPER_INDEX].slow_delay = step_interval_ticks;
      moves->moveCommands[X_STEPPER_INDEX].slow_delay_count = x_magnitude_steps-1;
    }
    moves->moveCommands[X_STEPPER_INDEX].direction = x_dir;

    #if DEBUG_STEP_GENERATION
    #if TESTBUILD
    sersendf_P( "\n  " );
    if( moves->moveCommands[X_STEPPER_INDEX].initial_delay
        + ( moves->moveCommands[X_STEPPER_INDEX].fast_delay_count * moves->moveCommands[X_STEPPER_INDEX].fast_delay)
        + ( moves->moveCommands[X_STEPPER_INDEX].slow_delay_count * moves->moveCommands[X_STEPPER_INDEX].slow_delay) != use_move_duration_ticks )
    {
        std::cout << "\n***** x error *****\n";
    }
    #endif
    sersendf_P( "x: steps=%d sit=%d sir=%d idt=%d fdt=%d fdc=%d sdt=%d sdc=%d dir=%d ", 
       x_magnitude_steps, 
       step_interval_ticks, 
       step_interval_remainder, 
       moves->moveCommands[X_STEPPER_INDEX].initial_delay,
       moves->moveCommands[X_STEPPER_INDEX].fast_delay,
       moves->moveCommands[X_STEPPER_INDEX].fast_delay_count, 
       moves->moveCommands[X_STEPPER_INDEX].slow_delay,
       moves->moveCommands[X_STEPPER_INDEX].slow_delay_count,
       moves->moveCommands[X_STEPPER_INDEX].direction );
    #endif
  }
  
  if( y_magnitude_steps )
  {
    unsigned int step_interval_ticks = use_move_duration_ticks / y_magnitude_steps;
    unsigned int step_interval_remainder = use_move_duration_ticks % y_magnitude_steps;
    
    moves->moveCommands[Y_STEPPER_INDEX].state = 1;
    moves->moveCommands[Y_STEPPER_INDEX].initial_delay = step_interval_ticks;
    if( step_interval_remainder )
    {
      moves->moveCommands[Y_STEPPER_INDEX].fast_delay = step_interval_ticks;
      moves->moveCommands[Y_STEPPER_INDEX].fast_delay_count = y_magnitude_steps - step_interval_remainder - 1;
      moves->moveCommands[Y_STEPPER_INDEX].slow_delay = step_interval_ticks + 1;
      moves->moveCommands[Y_STEPPER_INDEX].slow_delay_count = step_interval_remainder;
    }
    else
    {
      moves->moveCommands[Y_STEPPER_INDEX].slow_delay = step_interval_ticks;
      moves->moveCommands[Y_STEPPER_INDEX].slow_delay_count = y_magnitude_steps-1;
    }
    moves->moveCommands[Y_STEPPER_INDEX].direction = y_dir;

    #if DEBUG_STEP_GENERATION
    #if TESTBUILD
    sersendf_P( "\n  " );
    if( moves->moveCommands[Y_STEPPER_INDEX].initial_delay
        + ( moves->moveCommands[Y_STEPPER_INDEX].fast_delay_count * moves->moveCommands[Y_STEPPER_INDEX].fast_delay)
        + ( moves->moveCommands[Y_STEPPER_INDEX].slow_delay_count * moves->moveCommands[Y_STEPPER_INDEX].slow_delay) != use_move_duration_ticks )
    {
        std::cout << "\n***** y error *****\n";
    }
    #endif
    sersendf_P( "y: steps=%d sit=%d sir=%d idt=%d fdt=%d fdc=%d sdt=%d sdc=%d dir=%d ", 
       y_magnitude_steps, 
       step_interval_ticks, 
       step_interval_remainder, 
       moves->moveCommands[Y_STEPPER_INDEX].initial_delay,
       moves->moveCommands[Y_STEPPER_INDEX].fast_delay,
       moves->moveCommands[Y_STEPPER_INDEX].fast_delay_count, 
       moves->moveCommands[Y_STEPPER_INDEX].slow_delay,
       moves->moveCommands[Y_STEPPER_INDEX].slow_delay_count,
       moves->moveCommands[Y_STEPPER_INDEX].direction );
    #endif
  }
  
  if( z_magnitude_steps )
  {
    unsigned int step_interval_ticks = use_move_duration_ticks / z_magnitude_steps;
    unsigned int step_interval_remainder = use_move_duration_ticks % z_magnitude_steps;
    
    moves->moveCommands[Z_STEPPER_INDEX].state = 1;
    moves->moveCommands[Z_STEPPER_INDEX].initial_delay = step_interval_ticks;
    if( step_interval_remainder )
    {
      moves->moveCommands[Z_STEPPER_INDEX].fast_delay = step_interval_ticks;
      moves->moveCommands[Z_STEPPER_INDEX].fast_delay_count = z_magnitude_steps - step_interval_remainder - 1;
      moves->moveCommands[Z_STEPPER_INDEX].slow_delay = step_interval_ticks + 1;
      moves->moveCommands[Z_STEPPER_INDEX].slow_delay_count = step_interval_remainder;
    }
    else
    {
      moves->moveCommands[Z_STEPPER_INDEX].slow_delay = step_interval_ticks;
      moves->moveCommands[Z_STEPPER_INDEX].slow_delay_count = z_magnitude_steps-1;
    }
    moves->moveCommands[Z_STEPPER_INDEX].direction = z_dir;

    #if DEBUG_STEP_GENERATION
    #if TESTBUILD
    sersendf_P( "\n  " );
    if( moves->moveCommands[Z_STEPPER_INDEX].initial_delay
        + ( moves->moveCommands[Z_STEPPER_INDEX].fast_delay_count * moves->moveCommands[Z_STEPPER_INDEX].fast_delay)
        + ( moves->moveCommands[Z_STEPPER_INDEX].slow_delay_count * moves->moveCommands[Z_STEPPER_INDEX].slow_delay) != use_move_duration_ticks )
    {
        std::cout << "\n***** z error *****\n";
    }
    #endif
    sersendf_P( "z: steps=%d sit=%d sir=%d idt=%d fdt=%d fdc=%d sdt=%d sdc=%d dir=%d ", 
       z_magnitude_steps, 
       step_interval_ticks, 
       step_interval_remainder, 
       moves->moveCommands[Z_STEPPER_INDEX].initial_delay,
       moves->moveCommands[Z_STEPPER_INDEX].fast_delay,
       moves->moveCommands[Z_STEPPER_INDEX].fast_delay_count, 
       moves->moveCommands[Z_STEPPER_INDEX].slow_delay,
       moves->moveCommands[Z_STEPPER_INDEX].slow_delay_count,
       moves->moveCommands[Z_STEPPER_INDEX].direction );
    #endif
  }
  
  if( e_magnitude_steps )
  {
    unsigned int step_interval_ticks = use_move_duration_ticks / e_magnitude_steps;
    unsigned int step_interval_remainder = use_move_duration_ticks % e_magnitude_steps;
    
    moves->moveCommands[E_STEPPER_INDEX].state = 1;
    moves->moveCommands[E_STEPPER_INDEX].initial_delay = step_interval_ticks;
    if( step_interval_remainder )
    {
      moves->moveCommands[E_STEPPER_INDEX].fast_delay = step_interval_ticks;
      moves->moveCommands[E_STEPPER_INDEX].fast_delay_count = e_magnitude_steps - step_interval_remainder - 1;
      moves->moveCommands[E_STEPPER_INDEX].slow_delay = step_interval_ticks + 1;
      moves->moveCommands[E_STEPPER_INDEX].slow_delay_count = step_interval_remainder;
    }
    else
    {
      moves->moveCommands[E_STEPPER_INDEX].slow_delay = step_interval_ticks;
      moves->moveCommands[E_STEPPER_INDEX].slow_delay_count = e_magnitude_steps-1;
    }
    moves->moveCommands[E_STEPPER_INDEX].direction = e_dir;

    #if DEBUG_STEP_GENERATION
    #if TESTBUILD
    sersendf_P( "\n  " );
    if( moves->moveCommands[E_STEPPER_INDEX].initial_delay
        + ( moves->moveCommands[E_STEPPER_INDEX].fast_delay_count * moves->moveCommands[E_STEPPER_INDEX].fast_delay)
        + ( moves->moveCommands[E_STEPPER_INDEX].slow_delay_count * moves->moveCommands[E_STEPPER_INDEX].slow_delay) != use_move_duration_ticks )
    {
        std::cout << "\n***** e error *****\n";
    }
    #endif
    sersendf_P( "e: steps=%d sit=%d sir=%d idt=%d fdt=%d fdc=%d sdt=%d sdc=%d dir=%d ", 
       e_magnitude_steps, 
       step_interval_ticks, 
       step_interval_remainder, 
       moves->moveCommands[E_STEPPER_INDEX].initial_delay,
       moves->moveCommands[E_STEPPER_INDEX].fast_delay,
       moves->moveCommands[E_STEPPER_INDEX].fast_delay_count, 
       moves->moveCommands[E_STEPPER_INDEX].slow_delay,
       moves->moveCommands[E_STEPPER_INDEX].slow_delay_count,
       moves->moveCommands[E_STEPPER_INDEX].direction );
    #endif
  }

  #if TESTBUILD
  sersendf_P( "\n  " );
  #endif

  StepGenerator::ReleaseMove( moves );
  
  startpoint = *target;
  
  // next dda starts where we finish
  startpoint_steps.X = target_x_steps;
  startpoint_steps.Y = target_y_steps;
  startpoint_steps.Z = target_z_steps;
  startpoint_steps.E = target_e_steps;
 //memcpy(&startpoint, target, sizeof(TARGET));
  
  // if E is relative, reset it here
  if( E_AXIS_BEHAVIOR != E_AXIS_ABSOLUTE )
  {
    startpoint.E = 0; 
    startpoint_steps.E = 0;
  }
  
  if( E_AXIS_BEHAVIOR == E_AXIS_RELATIVE_ERROR_ACCUMULATE )
  {
    if( e_error_state == NORMAL )
    {
      if(e_dir == 1)
      {
          e_error =  e_delta_pu - e_magnitude_steps_pu * (e_dir?1:-1);
      }
      else
      {
         e_error = 0;
         e_error_state = ZERO_NEXT;
      }
    }
    else if( e_request_delta_pu > 0 )
    {
      e_error = 0;
      e_error_state = NORMAL;
    }

    #if DEBUG_STEP_GENERATION
    sersendf_P( "eerr=%d ", e_error );
    #if TESTBUILD
    sersendf_P( "\n  " );
    #endif
    #endif
  }
}

void MotionPlanner::SetXPosition( int32_t pos )
{
  startpoint.X = /*current_position.X =*/ pos;
  startpoint_steps.X = ((int64_t)pos * (int64_t) STEPS_PER_M_X + ((pos>=0)?(POSITION_UNITS_PARTS_PER_METER/2):-(POSITION_UNITS_PARTS_PER_METER/2))) / POSITION_UNITS_PARTS_PER_METER;
}
void MotionPlanner::SetYPosition( int32_t pos )
{
  startpoint.Y = /*current_position.Y =*/ pos;
  startpoint_steps.Y = ((int64_t)pos * (int64_t) STEPS_PER_M_Y + ((pos>=0)?(POSITION_UNITS_PARTS_PER_METER/2):-(POSITION_UNITS_PARTS_PER_METER/2))) / POSITION_UNITS_PARTS_PER_METER;
}
void MotionPlanner::SetZPosition( int32_t pos )
{
  startpoint.Z = /*current_position.X =*/ pos;
  startpoint_steps.Z = ((int64_t)pos * (int64_t) STEPS_PER_M_Z + ((pos>=0)?(POSITION_UNITS_PARTS_PER_METER/2):-(POSITION_UNITS_PARTS_PER_METER/2))) / POSITION_UNITS_PARTS_PER_METER;
}
 void MotionPlanner::SetEPosition( int32_t pos )
{
  startpoint.E = /*current_position.X =*/ pos;
  startpoint_steps.E = ((int64_t)pos * (int64_t) STEPS_PER_M_E + ((pos>=0)?(POSITION_UNITS_PARTS_PER_METER/2):-(POSITION_UNITS_PARTS_PER_METER/2))) / POSITION_UNITS_PARTS_PER_METER;
  if( E_AXIS_BEHAVIOR == E_AXIS_RELATIVE_ERROR_ACCUMULATE )
  {
    e_error = 0;
    //e_error_state = ?
  }
}




