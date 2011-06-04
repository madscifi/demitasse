#ifndef	_GCODE_PARSE_H
#define	_GCODE_PARSE_H

#ifndef _WIN32
#include	<stdint.h>
#endif

#include	"motion_planner.h"

// wether to insist on N line numbers
// if not defined, N's are completely ignored
//#define	REQUIRE_LINENUMBER

// wether to insist on a checksum
//#define	REQUIRE_CHECKSUM

extern const uint32_t powers[];  

// A simple parser of floating point numbers. This is based on 
// decfloat from Teacup.
//
// 
class NumericParser
{
public:
  NumericParser()
  {
    Clear();
  }

  void Clear()
  {
    mantissa = 0;
    exponent = 0;
    negative = false;
    clean = true;
  }

  bool AddChar( uint8_t c )
  {
      if (c >= '0' && c <= '9') 
      {
        clean = false;
        if( mantissa >= 100000000 )
        {
          if( exponent ) return true; // ignore extra digits after decimal point
          return false;               // number is too large to represent.
        }
        mantissa = mantissa * 10 + (c - '0');
        if (exponent) exponent++;
    }
    else if( c == '-' )
    {
      if( !clean ) return false;
      clean = false;
      negative = true;
    }
    else if( c == '.' )
    {
      clean = false;
      if( exponent != 0 ) return false;
      exponent = 1;
    }
    return true;
  }

  bool Parse( char *s )
  {
    Clear();
    bool valid = true;
    while( *s && ( valid = AddChar(*s++) ) ) ;
    return valid;
  }

  int32_t AsInt( bool fromInches )
  {
    int64_t m = mantissa;
    if( fromInches ) m = ((m * 254) + 5) / 10;
    int e = exponent;
    if( e > 1 ) e--;
    return m / powers[ e ];
  }

  int32_t AsPosition( bool fromInches )
  {
    int64_t m = mantissa;
    int e = exponent;
    if( e > 1 ) e--;
    if( fromInches ) m = m * 254;
    int32_t r = (m * (POSITION_UNITS_PARTS_PER_METER/1000) + powers[e] / 2) / powers[e];
    if( fromInches ) r = (r + 5) / 10;
    return negative ? -r : r;
  }

  int32_t AsScaledInt( int scale )
  {
    int e = exponent;
    if( e > 1 ) e--;
    int32_t r = (mantissa * scale + powers[e] / 2) / powers[e];
    return negative ? -r : r;
  }

private:
  uint32_t mantissa;		   ///< the actual digits of our floating point number
  uint32_t exponent;	     ///< scale mantissa by \f$10^{-exponent}\f$
  bool negative;     ///< positive or negative?
  bool clean;

};

/// this holds all the possible data from a received command
typedef struct {
	union {
		struct {
			uint8_t					seen_G	:1;
			uint8_t					seen_M	:1;
			uint8_t					seen_X	:1;
			uint8_t					seen_Y	:1;
			uint8_t					seen_Z	:1;
			uint8_t					seen_E	:1;
			uint8_t					seen_F	:1;
			uint8_t					seen_S	:1;

			uint8_t					seen_P	:1;
			uint8_t					seen_T	:1;
			uint8_t					seen_N	:1;
			uint8_t					seen_checksum				:1; ///< seen a checksum?
			uint8_t					seen_semi_comment		:1; ///< seen a semicolon?
			uint8_t					seen_parens_comment	:1; ///< seen an open parenthesis
			uint8_t					option_relative			:1; ///< relative or absolute coordinates?
			uint8_t					option_inches				:1; ///< inches or millimeters?
		};
		uint16_t				flags;
	};

	uint8_t						G;				///< G command number
	uint8_t						M;				///< M command number
	TARGET						target;		///< target position: X, Y, Z, E and F

	int16_t						S;				///< S word (various uses)
	uint16_t					P;				///< P word (various uses)

	uint8_t						T;				///< T word (tool index)

	uint32_t					N;				///< line number
	uint32_t					N_expected;	///< expected line number

	uint8_t						checksum_read;				///< checksum in gcode command
	uint8_t						checksum_calculated;	///< checksum we calculated
} GCODE_COMMAND;

/// the command being processed
extern GCODE_COMMAND next_target;

/// accept the next character and process it
void gcode_parse_char(uint8_t c);

#endif	/* _GCODE_PARSE_H */
