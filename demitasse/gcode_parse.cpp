#include	"gcode_parse.h"

/** \file
	\brief Parse received G-Codes
*/

#include "stdio.h"
#include <string.h>

#include "gcode_process.h"
#include "step_generator.h"

/// current or previous gcode word
/// for working out what to do with data just received
uint8_t last_field = 0;

/// crude crc macro
#define crc(a, b)		(a ^ b)

NumericParser parse_number; /// crude floating point data storage

/// this is where we store all the data for the current command before we work out what to do with it
GCODE_COMMAND next_target;

/*
	utility functions
*/
extern const uint32_t powers[];  // defined in sermsg.c

/// Character Received - add it to our command
/// \param c the next character to process
void gcode_parse_char(uint8_t c) 
{
	// uppercase
	if (c >= 'a' && c <= 'z')
		c &= ~32;

	// process previous field
	if (last_field) {
		// check if we're seeing a new field or end of line
		// any character will start a new field, even invalid/unknown ones
		if ((c >= 'A' && c <= 'Z') || c == '*' || (c == 10) || (c == 13)) {
			switch (last_field) {
				case 'G':
					next_target.G = parse_number.AsInt( false );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint8(next_target.G);
					break;
				case 'M':
					next_target.M = parse_number.AsInt( false );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint8(next_target.M);
					break;
				case 'X':
					next_target.target.X = parse_number.AsPosition( next_target.option_inches );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_int32(next_target.target.X);
					break;
				case 'Y':
					next_target.target.Y = parse_number.AsPosition( next_target.option_inches );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_int32(next_target.target.Y);
					break;
				case 'Z':
					next_target.target.Z = parse_number.AsPosition( next_target.option_inches );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_int32(next_target.target.Z);
					break;
				case 'E':
					next_target.target.E = parse_number.AsPosition( next_target.option_inches );
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint32(next_target.target.E);
					break;
				case 'F':
					// just use raw integer, we need move distance and n_steps to convert it to a useful value, so wait until we have those to convert it
					next_target.target.F = parse_number.AsInt( next_target.option_inches );
					//if (next_target.option_inches)
					//	next_target.target.F = decfloat_to_int(&read_digit, 25400, 1);
					//else
					//	next_target.target.F = decfloat_to_int(&read_digit, 1, 0);
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint32(next_target.target.F);
					break;
				case 'S':
					// if this is temperature, multiply by 4 to convert to quarter-degree units
					// cosmetically this should be done in the temperature section,
					// but it takes less code, less memory and loses no precision if we do it here instead
					if ((next_target.M == 104) || (next_target.M == 109) || (next_target.M == 140))
						next_target.S = parse_number.AsScaledInt( 4 ); //decfloat_to_int(&read_digit, 4, 0);
					// if this is heater PID stuff, multiply by PID_SCALE because we divide by PID_SCALE later on
					else if ((next_target.M >= 130) && (next_target.M <= 132))
						next_target.S = parse_number.AsScaledInt( PID_SCALE );// decfloat_to_int(&read_digit, PID_SCALE, 0);
					else
						next_target.S = parse_number.AsInt( false ); //decfloat_to_int(&read_digit, 1, 0);
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint16(next_target.S);
					break;
				case 'P':
					next_target.P = parse_number.AsInt( false ); //decfloat_to_int(&read_digit, 1, 0);
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint16(next_target.P);
					break;
				case 'T':
					next_target.T = parse_number.AsInt( false ); //read_digit.mantissa;
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint8(next_target.T);
					break;
				case 'N':
					next_target.N = parse_number.AsInt( false ); //decfloat_to_int(&read_digit, 1, 0);
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint32(next_target.N);
					break;
				case '*':
					next_target.checksum_read = parse_number.AsInt( false ); //decfloat_to_int(&read_digit, 1, 0);
					//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
					//	serwrite_uint8(next_target.checksum_read);
					break;
			}
			// reset for next field
			last_field = 0;
			parse_number.Clear();
		}
	}

	// skip comments
	if (next_target.seen_semi_comment == 0 && next_target.seen_parens_comment == 0) {
		// new field?
		if ((c >= 'A' && c <= 'Z') || c == '*') {
			last_field = c;
			//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
			//	serial_writechar(c);
		}

		// process character
		switch (c) {
			// each currently known command is either G or M, so preserve previous G/M unless a new one has appeared
			// FIXME: same for T command
			case 'G':
				next_target.seen_G = 1;
				next_target.seen_M = 0;
				next_target.M = 0;
				break;
			case 'M':
				next_target.seen_M = 1;
				next_target.seen_G = 0;
				next_target.G = 0;
				break;
			case 'X':
				next_target.seen_X = 1;
				break;
			case 'Y':
				next_target.seen_Y = 1;
				break;
			case 'Z':
				next_target.seen_Z = 1;
				break;
			case 'E':
				next_target.seen_E = 1;
				break;
			case 'F':
				next_target.seen_F = 1;
				break;
			case 'S':
				next_target.seen_S = 1;
				break;
			case 'P':
				next_target.seen_P = 1;
				break;
			case 'T':
				next_target.seen_T = 1;
				break;
			case 'N':
				next_target.seen_N = 1;
				break;
			case '*':
				next_target.seen_checksum = 1;
				break;

			// comments
			case ';':
				next_target.seen_semi_comment = 1;
				break;
			case '(':
				next_target.seen_parens_comment = 1;
				break;

			// now for some numeracy
			case '-':
				parse_number.AddChar(c);
				break;
			case '.':
				parse_number.AddChar(c);
				break;
			#ifdef	DEBUG
			case ' ':
			case '\t':
			case 10:
			case 13:
				// ignore
				break;
			#endif

			default:
				parse_number.AddChar(c);
		}
	} else if ( next_target.seen_parens_comment == 1 && c == ')')
		next_target.seen_parens_comment = 0; // recognize stuff after a (comment)

	if (next_target.seen_checksum == 0)
		next_target.checksum_calculated = crc(next_target.checksum_calculated, c);

	// end of line
	if ((c == 10) || (c == 13)) {
		//if (DEBUG_ECHO && (debug_flags & DEBUG_ECHO))
		//	serial_writechar(c);

		if (
		#ifdef	REQUIRE_LINENUMBER
			((next_target.N >= next_target.N_expected) && (next_target.seen_N == 1)) ||
			(next_target.seen_M && (next_target.M == 110))
		#else
			1
		#endif
			) {
			if (
				#ifdef	REQUIRE_CHECKSUM
				((next_target.checksum_calculated == next_target.checksum_read) && (next_target.seen_checksum == 1))
				#else
				((next_target.checksum_calculated == next_target.checksum_read) || (next_target.seen_checksum == 0))
				#endif
				) {
				// process
                                // teacup waits until after the processing of the gcode to send the eol...
                                // FIXME - what to do here...
				serial_writestr_P(PSTR("ok "));
				process_gcode_command();
				serial_writechar('\n');

				// expect next line number
				if (next_target.seen_N == 1)
					next_target.N_expected = next_target.N + 1;
			}
			else {
				sersendf_P(PSTR("rs N%ld Expected checksum %d\n"), next_target.N_expected, next_target.checksum_calculated);
// 				request_resend();
			}
		}
		else {
			sersendf_P(PSTR("rs N%ld Expected line number %ld\n"), next_target.N_expected, next_target.N_expected);
// 			request_resend();
		}

		// reset variables
		next_target.seen_X = next_target.seen_Y = next_target.seen_Z = \
			next_target.seen_E = next_target.seen_F = next_target.seen_S = \
			next_target.seen_P = next_target.seen_T = next_target.seen_N = \
			next_target.seen_M = next_target.seen_checksum = next_target.seen_semi_comment = \
			next_target.seen_parens_comment = next_target.checksum_read = \
			next_target.checksum_calculated = 0;
		// last_field and read_digit are reset above already

		// assume a G1 by default
		next_target.seen_G = 1;
		next_target.G = 1;

		if (next_target.option_relative) {
			next_target.target.X = next_target.target.Y = next_target.target.Z = 0;
			#ifdef	E_ABSOLUTE
				next_target.target.E = 0;
			#endif
		}
		#ifndef	E_ABSOLUTE
			// E always relative
			next_target.target.E = 0;
		#endif
	}
}

/// list of powers of ten, used for dividing down decimal numbers for sending, and also for our crude floating point algorithm
const uint32_t powers[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

