#ifndef	MOTION_PLANNER_H
#define	MOTION_PLANNER_H

#include "config.h"
#include "processor.h"

#ifndef _WIN32
#include <stdint.h>
#endif

/*
	types
*/

/**
	\struct TARGET
	\brief target is simply a point in space/time

	X, Y, Z and E are in micrometers unless explcitely stated. F is in mm/min.
*/
typedef struct {
	int32_t						X;
	int32_t						Y;
	int32_t						Z;
	int32_t						E;
	uint32_t					F;
} TARGET;

struct MotionPlanner
{
  static TARGET startpoint;
  static TARGET startpoint_steps;

  enum e_error_states { NORMAL, ZERO_NEXT };
  static e_error_states e_error_state;
  static int32_t e_error;
  
  static void Init();
  
  static void ScheduleMovement( TARGET* target );
  static void SetXPosition( int32_t pos );
  static void SetYPosition( int32_t pos );
  static void SetZPosition( int32_t pos );
  static void SetEPosition( int32_t pos );

};

#endif	/* _DDA_H */
