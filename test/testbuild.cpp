#include "processor.h"
#include "buffered_serial.h"
#include "gcode_parse.h"

int f1( int t )
{
    return (((int64_t)t * (int64_t) 5155509) / 100000 + ((t>=0)?5:-5)) / 10L;
}
int f2( int t )
{
    return ((int64_t)t * (int64_t) 5155509 + ((t>=0)?500000:-500000)) / 1000000;
}


int main(int argc, char**argv )
{
  NumericParser f;
  f.Parse("999");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("999.001");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("999.000001");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("1.000001");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("1.0000015");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("1.0000005");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("1.0000004");
  std::cout << f.AsInt(false) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";
  f.Parse("1.0000004");
  std::cout << f.AsInt(true) << " " << f.AsPosition( false ) << " " << f.AsPosition( true ) << "\n";

/*
				if (c >= '0' && c <= '9') {
					if (read_digit.exponent < DECFLOAT_EXP_MAX &&
							((next_target.option_inches == 0 &&
							read_digit.mantissa < DECFLOAT_MANT_MM_MAX) ||
							(next_target.option_inches &&
							read_digit.mantissa < DECFLOAT_MANT_IN_MAX)))
					{
						// this is simply mantissa = (mantissa * 10) + atoi(c) in different clothes
						read_digit.mantissa = (read_digit.mantissa << 3) + (read_digit.mantissa << 1) + (c - '0');
						if (read_digit.exponent)
							read_digit.exponent++;
					}
				}
*/


    //gSerial.Init( "octopusv10block_fixed_sc055 - half size_export.gcode" );
  MotionPlanner::Init();
    gSerial.Init( "test.gcode" );
    int c;
    while( (c = gSerial.GetCharNoBlocking()) != -1 )
    {
        std::cout << (unsigned char)c;
        gcode_parse_char( c );
    }
    return 0;
}

