#include "utils.h"

SettableOption* SettableOption::options[SettableOption::OPTION_ID_END];

namespace Option 
{
  
SettableIntOption x_max_feedrate_mm_m( SettableOption::X_MAX_FEEDRATE, Config::MAXIMUM_FEEDRATE_X_MM_M );
SettableIntOption y_max_feedrate_mm_m( SettableOption::Y_MAX_FEEDRATE, Config::MAXIMUM_FEEDRATE_Y_MM_M );
SettableIntOption z_max_feedrate_mm_m( SettableOption::Z_MAX_FEEDRATE, Config::MAXIMUM_FEEDRATE_Z_MM_M );
SettableIntOption e_max_feedrate_mm_m( SettableOption::E_MAX_FEEDRATE, Config::MAXIMUM_FEEDRATE_E_MM_M );

SettableIntOption x_search_feedrate_mm_m( SettableOption::X_SEARCH_FEEDRATE, Config::SEARCH_FEEDRATE_X );
SettableIntOption y_search_feedrate_mm_m( SettableOption::Y_SEARCH_FEEDRATE, Config::SEARCH_FEEDRATE_Y );
SettableIntOption z_search_feedrate_mm_m( SettableOption::Z_SEARCH_FEEDRATE, Config::SEARCH_FEEDRATE_Z );
SettableIntOption e_search_feedrate_mm_m( SettableOption::E_SEARCH_FEEDRATE, Config::SEARCH_FEEDRATE_E );

SettableIntOption x_steps_per_meter( SettableOption::X_STEPS_PER_METER, Config::STEPS_PER_M_X );
SettableIntOption y_steps_per_meter( SettableOption::Y_STEPS_PER_METER, Config::STEPS_PER_M_Y );
SettableIntOption z_steps_per_meter( SettableOption::Z_STEPS_PER_METER, Config::STEPS_PER_M_Z );
SettableIntOption e_steps_per_meter( SettableOption::E_STEPS_PER_METER, Config::STEPS_PER_M_E );

SettableIntZeroOneOption x_invert_dir(SettableOption::X_INVERT_DIR, Config::X_INVERT_DIR);
SettableIntZeroOneOption y_invert_dir(SettableOption::Y_INVERT_DIR, Config::Y_INVERT_DIR);
SettableIntZeroOneOption z_invert_dir(SettableOption::Z_INVERT_DIR, Config::Z_INVERT_DIR);
SettableIntZeroOneOption e_invert_dir(SettableOption::E_INVERT_DIR, Config::E_INVERT_DIR);

}
