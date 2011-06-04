Introduction
------------

Demitasse is an experimental controller for Reprap that is derived from 
Teacup (http://reprap.org/wiki/Teacup_Firmware) and runs on a ChipKit 
Uno32. At present it is only partially functional - it can drive the x, 
y, z and e steppers but not heaters nor can it read temp sensors. With 
that caveat it has successfully printed a handful of 50%-sized octopi 
(thingiverse thing:7900) in conjunction with a separate temp controller 
that was used to control the extruder's heater.

Overview
--------

The code is a mixture of C++ and C code. The current plan is to move 
everything into C++ code (templates and objects, but no virtual functions,
dynamic memory allocation or exceptions). It compiles with Multi-Platform
Version of the Arduino IDE (https://github.com/chipKIT32/chipKIT32-MAX).

The GCODE parser is taken directly from Teacup, and the motion scheduler 
strongly resembles the one in Teacup. These sections have been modified to
track position by um and to compute step rates based on the distance 
actually traversed by each step. This is what Traumflug has started on 
Teacup but it is much easier to implement completely on the Pic32 chips 
than it is the Atmega chips without due to the practical availability of 
64 bit operators.

The step pulse generation is where the code deviates significantly from 
Teacup (and all of the other Reprap firmware that I am familiar with). The
output capture hardware is used to generate the rising edge of of the step
pulse such that the edge always occurs precisely at the intended time, to
the resolution of the timer (currently 200ns). The rising edge also 
generates an interrupt and the code uses the interrupt to end the step 
pulse and to set up the output capture unit for the next step. Each 
stepper is controlled by a separate output capture unit.

The Pic32 chip on the Uno32 contains 5 output capture units, so this
approach is limited to driving 5 stepper motors directly. While this 
approach generates pulse streams that look absolutely beautiful on a 
scope, I've observed no advantage in an actual print. Where this approach 
might have some actual advantage is in case of a very high pulse rate. 
However, such pulse rates can only be achieved with acceleration and at 
present this firmware does not support acceleration.

What Works
----------

G1, G21, G90, G91 and G92 all work, mostly. G20 should work, but it has 
not been tested. The code is probably full of overflow and other issues at 
this point, so beware. Most other commands supported by Teacup are 
accepted and silently ignored. Some of the other commands may cause 
things to break, so it is best to stick to the ones listed at the moment.

The firmware expects the E axis values in the GCODE to be relative, even
if the x/y/z positions are absolute. 

The only flow control supported is the simple case of sending single lines
and waiting for an "ok" before sending the next line. 

What is Missing
---------------

Heaters
Temp sensors
Min/Max/Home switches 
Home commands
Debug Logging
Serial flow control (xon/xoff)
Acceleration

Other Issues
------------

The boot loader on the Uno32 delays 4 seconds at startup. This is 
rather annoying and needs to be addressed in some fashion.

The hardware output compare functionality is tied to specific pins,
so this will probably never be pin compatible with any existing 
Arduino stepper board.

The Uno32 board runs at 3.3 volts which might be a problem with some
stepper drivers. Pololu drivers will work at 3.3 volts, but the current 
control potentiometer is based off the supply voltage, so the 
potentiometer will most likely need to be adjusted.


Project Layout
--------------

The demitasse folder contains demitasse.pde, which can be opened in 
mpide to build the project.

The test folder contains various test files and VC6 projects used for
testing various components on a desktop. The contents are not needed
is you are planning on simply using the firmware.

Signals
-------
The follow pinout refers to the pin labels on the Uno32 board. Note that
the step pins cannot be remapped (except to other step pins) since they
depend on the output compare units.

Digital Pins
2 - x direction
3 - x step
4 - y direction
5 - y step
6 - z step
7 - z direction
8 - e direction
9 - e step
11 - stepper enable (turns on after reset and stays on)

Analog Pins
A0 - currently used as a digital output for debugging purposes

