#ifndef	UTILS_H_
#define	UTILS_H_

#include "config.h"

#ifndef _WIN32
#include	<stdint.h>
#endif

struct SettableOption
{
  enum OptionId
  {
    X_MAX_FEEDRATE = 1,
    Y_MAX_FEEDRATE,
    Z_MAX_FEEDRATE,
    E_MAX_FEEDRATE,
    
    X_SEARCH_FEEDRATE,
    Y_SEARCH_FEEDRATE,
    Z_SEARCH_FEEDRATE,
    E_SEARCH_FEEDRATE,

    X_STEPS_PER_METER,
    Y_STEPS_PER_METER,
    Z_STEPS_PER_METER,
    E_STEPS_PER_METER,
    
    X_INVERT_DIR,
    Y_INVERT_DIR,
    Z_INVERT_DIR,
    E_INVERT_DIR,
    
    OPTION_ID_END
  };
  
  SettableOption( OptionId id )
  : id( id )
  {
    options[id] = this;
  }
  
  uint32_t id;
  
  static SettableOption* options[];
  
  static void SetOptionValue( int id, int32_t value )
  {
    if( id >= 0 && id < OPTION_ID_END && options[id] )
    {
      options[id]->SetValue( value );
    }
  }
  
  static double GetOptionValue( int id )
  {
    if( id >= 0 && id < OPTION_ID_END && options[id] )
    {
      return options[id]->GetValue();
    }
  }

  static void ResetOptionValue( int id )
  {
    if( id >= 0 && id < OPTION_ID_END && options[id] )
    {
      options[id]->ResetValue();
    }
  }
  
  static void ResetAllValues()
  {
    for( int id = 0; id < OPTION_ID_END; ++id )
    {
      if( options[id] )
      {
        options[id]->ResetValue();
      }
    }
  }
  
  virtual void SetValue( int32_t value ) = 0;
  virtual double GetValue() const = 0;
  virtual void ResetValue() = 0;
};

struct SettableIntOption : public SettableOption
{
  uint32_t mValue;
  uint32_t mDefault;
  
  SettableIntOption( OptionId id, uint32_t defaultValue )
  : SettableOption( id )
  {
    mValue = defaultValue;
    mDefault = defaultValue;
  }
  
  operator uint32_t()
  {
    return mValue;
  }
  
  virtual void SetValue( int32_t value )
  {
    mValue = value;
  }
  
  virtual double GetValue() const
  {
    return mValue;
  }
  
  virtual void ResetValue()
  {
    mValue = mDefault;
  }
  
};

struct SettableIntZeroOneOption : public SettableOption
{
  uint32_t mValue;
  uint32_t mDefault;
  
  SettableIntZeroOneOption( OptionId id, uint32_t defaultValue )
  : SettableOption( id )
  {
    mValue = defaultValue;
    mDefault = defaultValue;
  }
  
  operator uint32_t()
  {
    return mValue;
  }
  
  virtual void SetValue( int32_t value )
  {
    mValue = (value?1:0);
  }
  
  virtual double GetValue() const
  {
    return mValue;
  }
  
  virtual void ResetValue()
  {
    mValue = mDefault;
  }
};

struct SettablePositionOption : public SettableOption
{
};


namespace Option
{

extern SettableIntOption x_max_feedrate_mm_m;
extern SettableIntOption y_max_feedrate_mm_m;
extern SettableIntOption z_max_feedrate_mm_m;
extern SettableIntOption e_max_feedrate_mm_m;

extern SettableIntOption x_search_feedrate_mm_m;
extern SettableIntOption y_search_feedrate_mm_m;
extern SettableIntOption z_search_feedrate_mm_m;
extern SettableIntOption e_search_feedrate_mm_m;

extern SettableIntOption x_steps_per_meter;
extern SettableIntOption y_steps_per_meter;
extern SettableIntOption z_steps_per_meter;
extern SettableIntOption e_steps_per_meter;

extern SettableIntZeroOneOption x_invert_dir;
extern SettableIntZeroOneOption y_invert_dir;
extern SettableIntZeroOneOption z_invert_dir;
extern SettableIntZeroOneOption e_invert_dir;

} // namespace Option

#endif	/* UTILS_H_ */
