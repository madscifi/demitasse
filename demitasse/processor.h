#ifndef TESTBUILD

    #include <stdint.h>
    #define MEMORY_BARRIER()   __asm volatile( "" ::: "memory" )
    #define DISABLE_INTERRUPTS() __asm volatile( "di" ::: "memory" )
    #define ENABLE_INTERRUPTS() __asm volatile( "ei" ::: "memory" )

#else

    #define MEMORY_BARRIER()
    #define DISABLE_INTERRUPTS()
    #define ENABLE_INTERRUPTS()

    typedef unsigned int uint32_t;
    typedef int int32_t;
    typedef unsigned char uint8_t;
    typedef signed char int8_t;
    typedef unsigned short uint16_t;
    typedef short int16_t;
    typedef __int64 int64_t;
    //typedef __uint64 uint64_t;

    #define __attribute__(x)

    #define IoPortId uint8_t

    #define vsnprintf _vsnprintf
    #define PORTSetPinsDigitalOut(x,y)
    #define PORTSetPinsDigitalIn(x,y)
    #define PORTClearBits(x,y)
    #define PORTSetBits(x,y)

#endif
