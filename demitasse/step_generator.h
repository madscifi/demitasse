#ifndef STEP_GENERATOR_H_
#define STEP_GENERATOR_H_

#ifndef TESTBUILD
#include <plib.h>
#endif

#include "port_pin.h"
#include "buffered_serial.h"

#include "ring_buffer.h"
#include "utils.h"

enum { X_STEPPER_INDEX, Y_STEPPER_INDEX, Z_STEPPER_INDEX, E_STEPPER_INDEX, STEPPER_COUNT };

struct Stats
{
  static uint32_t bmCount;
  
  static void Display( bool reset )
  {
    sersendf_P("bm:%x ", bmCount );
    if( reset )
    {
      bmCount = 0;
    }
  }
  
  static void IncBM()
  {
    ++bmCount;
  }

};

struct StepGenerator
{
  struct StepperMove
  {
    // all delays expressed in timer2 ticks - currently 0.2us
    unsigned int state;
    unsigned int direction;
    unsigned int initial_delay;  // zero if not used
    unsigned int fast_delay;
    unsigned int fast_delay_count;
    unsigned int slow_delay;
    unsigned int slow_delay_count;
  };
  
  struct StepperMoves
  {
    StepperMove moveCommands[ STEPPER_COUNT ];
  };
  
  typedef RingBuffer<StepperMoves,16> RingBufferType;
  static RingBufferType stepper_move_buffer;


  static unsigned int live_axis_count;
  static PortPin dirPins[ STEPPER_COUNT ]; 
   
#ifndef TESTBUILD

  static void Init()
  {
      // set port pins used by OC1-OC4 to output (value of zero).
      // This is necessary because setting the OC Mode to 000 returns
      // control of the output pin to the general io module, and the
      // code wants the output pin to be 0 at that point.
      
      PORTSetPinsDigitalOut( IOPORT_D, 0xf );
      
      for( int i = 0; i < STEPPER_COUNT; ++i )
      {
        dirPins[ i ].Init();
      }
      
      // Configure Timer2 for 32-bit operation
      // with a prescaler of 16. The Timer2/Timer3
      // pair is accessed via registers associated
      // with the Timer2 register
      
      // creates clock with period of 0.2us/tick
      // given an 80mhz system clock.
      T2CON = 0x0048;

      // turn off the oc hardware while doing setup      
      OC1CON = 0;
      OC2CON = 0;
      OC3CON = 0;
      OC4CON = 0;

      // enable single pulse mode
      OC1CON = _OC1CON_OC32_MASK; 
      OC2CON = _OC2CON_OC32_MASK;
      OC3CON = _OC3CON_OC32_MASK;
      OC4CON = _OC4CON_OC32_MASK;

      // Initialize primary Compare Register - not actually used
      // at this point...
      OC1R = 0x00003000;    
      OC2R = 0x00003000;    // Initialize primary Compare Register
      OC3R = 0x00003000;    // Initialize primary Compare Register
      OC4R = 0x00003000;    // Initialize primary Compare Register
      
      // Set rollover of counter
      PR2 = 0xffffffffUL;     
                            
      // clear all oc interrupt flags             
      IFS0CLR = _IFS0_OC1IF_MASK | _IFS0_OC2IF_MASK | _IFS0_OC3IF_MASK | _IFS0_OC4IF_MASK; // Clear the OC1 interrupt flag
      
      // set interrupt priorities and subpriorities
      IPC1SET = ((5 << _IPC1_OC1IP_POSITION) & _IPC1_OC1IP_MASK)
              | ((3 << _IPC1_OC1IS_POSITION) & _IPC1_OC1IS_MASK);

      IPC2SET = ((5 << _IPC2_OC2IP_POSITION) & _IPC2_OC2IP_MASK)
              | ((3 << _IPC2_OC2IS_POSITION) & _IPC2_OC2IS_MASK);

      IPC3SET = ((5 << _IPC3_OC3IP_POSITION) & _IPC3_OC3IP_MASK)
              | ((3 << _IPC3_OC3IS_POSITION) & _IPC3_OC3IS_MASK);

      IPC4SET = ((5 << _IPC4_OC4IP_POSITION) & _IPC4_OC4IP_MASK)
              | ((3 << _IPC4_OC4IS_POSITION) & _IPC4_OC4IS_MASK);

      // start timer2/3
      T2CONSET = _T2CON_ON_MASK; 

      // enable oc units
      OC1CONSET = _OC1CON_ON_MASK;
      OC2CONSET = _OC2CON_ON_MASK;
      OC3CONSET = _OC3CON_ON_MASK;
      OC4CONSET = _OC4CON_ON_MASK;
   }
   
   static bool IsFull()
   {
     MEMORY_BARRIER();
     return stepper_move_buffer.IsFull();
   }
   
   static StepperMoves* ReserveMove()
   {
     MEMORY_BARRIER();
     if( stepper_move_buffer.IsFull() )
     {
        return 0;
     }
     return stepper_move_buffer.reservePush();
   }
   
   static void ReleaseMove( StepperMoves* moves )
   {
     // make certain any writes to the buffer contents are flushed before 
     // exposing the data to the interrupt routines.
     DISABLE_INTERRUPTS();
     
     bool wasEmpty = stepper_move_buffer.IsEmpty();
     stepper_move_buffer.releasePush();
     
     // If the queue was empty before the move command was added then
     // there there are no stepper interrupts scheduled, so the stepper
     // motion must be started here. Otherwise, the motion will be automatically
     // executed by the interrupt functions at the appropriate time.
     if( wasEmpty )
     {
       BeginMove( moves, TMR2 );
     }
     
     ENABLE_INTERRUPTS();
     
     if( wasEmpty )
     {
       Stats::IncBM();
       gSerial.WriteStringBlocking( "[bm] " );
     }
   }
   
   static void BeginMove( StepperMoves* moves, unsigned int now )
   {
     live_axis_count = 0;
     
     IFS0CLR = _IFS0_OC1IF_MASK | _IFS0_OC2IF_MASK | _IFS0_OC3IF_MASK | _IFS0_OC4IF_MASK; // Clear the OC1 interrupt flag

     if( moves->moveCommands[X_STEPPER_INDEX].initial_delay )
     {
       dirPins[ X_STEPPER_INDEX ].SetTo( moves->moveCommands[X_STEPPER_INDEX].direction ^ Option::x_invert_dir );
       OC1R = now + moves->moveCommands[X_STEPPER_INDEX].initial_delay;
       OC1CONSET = _OC1CON_OCM0_MASK;
       IEC0SET = _IEC0_OC1IE_MASK;
       moves->moveCommands[X_STEPPER_INDEX].state = 1;
       ++live_axis_count;
     }
     
     if( moves->moveCommands[Y_STEPPER_INDEX].initial_delay )
     {
       dirPins[Y_STEPPER_INDEX].SetTo( moves->moveCommands[Y_STEPPER_INDEX].direction ^ Option::y_invert_dir );
       OC2R = now + moves->moveCommands[Y_STEPPER_INDEX].initial_delay;
       OC2CONSET = _OC2CON_OCM0_MASK;
       IEC0SET = _IEC0_OC2IE_MASK;
       moves->moveCommands[Y_STEPPER_INDEX].state = 1;
       ++live_axis_count;
     }
     if( moves->moveCommands[Z_STEPPER_INDEX].initial_delay )
     {
       dirPins[Z_STEPPER_INDEX].SetTo( moves->moveCommands[Z_STEPPER_INDEX].direction ^ Option::z_invert_dir );
       OC3R = now + moves->moveCommands[Z_STEPPER_INDEX].initial_delay;
       OC3CONSET = _OC3CON_OCM0_MASK;
       IEC0SET = _IEC0_OC3IE_MASK;
       moves->moveCommands[Z_STEPPER_INDEX].state = 1;
       ++live_axis_count;
     }
     if( moves->moveCommands[E_STEPPER_INDEX].initial_delay )
     {
       dirPins[E_STEPPER_INDEX].SetTo( moves->moveCommands[E_STEPPER_INDEX].direction ^ Option::e_invert_dir );
       OC4R = now + moves->moveCommands[E_STEPPER_INDEX].initial_delay;
       OC4CONSET = _OC4CON_OCM0_MASK;
       IEC0SET = _IEC0_OC4IE_MASK;
       moves->moveCommands[E_STEPPER_INDEX].state = 1;
       ++live_axis_count;
     }
   }
   
  static inline void Handle_OC1()
  {
    StepperMoves* moves = stepper_move_buffer.peekTail();
    StepperMove* move = &moves->moveCommands[X_STEPPER_INDEX];
  
    OC1CONCLR = _OC1CON_OCM_MASK;
   
    switch( move->state )
    {
      case 0:
        // should not happen
        IEC0CLR = _IEC0_OC1IE_MASK;
        break;
        
      case 1: // initial move
        move->state = 2;
        
      case 2: // fast delay
        if( move->fast_delay )
        {
          if( move->fast_delay_count )
          {
            OC1R =  (OC1R + move->fast_delay) ;    // Initialize primary Compare Register
            OC1CONSET = _OC1CON_OCM0_MASK;
            --move->fast_delay_count;
            break;
          }
        }
        move->state = 3;
           
      case 3: // slow delay
        if( move->slow_delay )
        {
          if( move->slow_delay_count )
          {
            OC1R =  (OC1R + move->slow_delay) ;    // Initialize primary Compare Register
            OC1CONSET = _OC1CON_OCM0_MASK;
            --move->slow_delay_count;
            break;
          }
        }
        move->state = 4;
        
      case 4: // last delay
        IEC0CLR = _IEC0_OC1IE_MASK;
        if( --live_axis_count == 0 )
        {
          // this is the last axis interrupt that will occur for this move command,
          // so release it and start up the next command if there is one.
          stepper_move_buffer.releaseTail();
          if( !stepper_move_buffer.IsEmpty() )
          {
            BeginMove( stepper_move_buffer.peekTail(), OC1R );
          }
        }
        move->state = 5;
        break;
    }  
    
    IFS0CLR = _IFS0_OC1IF_MASK; // Clear the OC1 interrupt flag
  }

  static inline void Handle_OC2()
  {
    StepperMoves* moves = stepper_move_buffer.peekTail();
    StepperMove* move = &moves->moveCommands[Y_STEPPER_INDEX];
  
    OC2CONCLR = _OC2CON_OCM_MASK;
   
    switch( move->state )
    {
      case 0:
        // should not happen
        IEC0CLR = _IEC0_OC2IE_MASK;
        break;
        
      case 1: // initial move
        move->state = 2;
        
      case 2: // fast delay
        if( move->fast_delay )
        {
          if( move->fast_delay_count )
          {
            OC2R =  (OC2R + move->fast_delay) ;    // Initialize primary Compare Register
            OC2CONSET = _OC2CON_OCM0_MASK;
            --move->fast_delay_count;
            break;
          }
        }
        move->state = 3;
           
      case 3: // slow delay
        if( move->slow_delay )
        {
          if( move->slow_delay_count )
          {
            OC2R =  (OC2R + move->slow_delay) ;    // Initialize primary Compare Register
            OC2CONSET = _OC2CON_OCM0_MASK;
            --move->slow_delay_count;
            break;
          }
        }
        move->state = 4;
        
      case 4: // last delay
        IEC0CLR = _IEC0_OC2IE_MASK;
        if( --live_axis_count == 0 )
        {
          // this is the last axis interrupt that will occur for this move command,
          // so release it and start up the next command if there is one.
          stepper_move_buffer.releaseTail();
          if( !stepper_move_buffer.IsEmpty() )
          {
            BeginMove( stepper_move_buffer.peekTail(), OC2R );
          }
        }
        move->state = 5;
        break;
    }  
    
       
    IFS0CLR = _IFS0_OC2IF_MASK; // Clear the OC1 interrupt flag
  }

  static inline void Handle_OC3()
  {
    StepperMoves* moves = stepper_move_buffer.peekTail();
    StepperMove* move = &moves->moveCommands[Z_STEPPER_INDEX];
  
    OC3CONCLR = _OC3CON_OCM_MASK;
   
    switch( move->state )
    {
      case 0:
        // should not happen
        IEC0CLR = _IEC0_OC3IE_MASK;
        break;
        
      case 1: // initial move
        move->state = 2;
        
      case 2: // fast delay
        if( move->fast_delay )
        {
          if( move->fast_delay_count )
          {
            OC3R =  (OC3R + move->fast_delay) ;    // Initialize primary Compare Register
            OC3CONSET = _OC3CON_OCM0_MASK;
            --move->fast_delay_count;
            break;
          }
        }
        move->state = 3;
           
      case 3: // slow delay
        if( move->slow_delay )
        {
          if( move->slow_delay_count )
          {
            OC3R =  (OC3R + move->slow_delay) ;    // Initialize primary Compare Register
            OC3CONSET = _OC3CON_OCM0_MASK;
            --move->slow_delay_count;
            break;
          }
        }
        move->state = 4;
        
      case 4: // last delay
        IEC0CLR = _IEC0_OC3IE_MASK;
        if( --live_axis_count == 0 )
        {
          // this is the last axis interrupt that will occur for this move command,
          // so release it and start up the next command if there is one.
          stepper_move_buffer.releaseTail();
          if( !stepper_move_buffer.IsEmpty() )
          {
            BeginMove( stepper_move_buffer.peekTail(), OC3R );
          }
        }
        move->state = 5;
        break;
    }  
    
       
    IFS0CLR = _IFS0_OC3IF_MASK; // Clear the OC1 interrupt flag
  }

  static inline void Handle_OC4()
  {
    StepperMoves* moves = stepper_move_buffer.peekTail();
    StepperMove* move = &moves->moveCommands[E_STEPPER_INDEX];
  
    OC4CONCLR = _OC4CON_OCM_MASK;
   
    switch( move->state )
    {
      case 0:
        // should not happen
        IEC0CLR = _IEC0_OC4IE_MASK;
        break;
        
      case 1: // initial move
        move->state = 2;
        
      case 2: // fast delay
        if( move->fast_delay )
        {
          if( move->fast_delay_count )
          {
            OC4R =  (OC4R + move->fast_delay) ;    // Initialize primary Compare Register
            OC4CONSET = _OC4CON_OCM0_MASK;
            --move->fast_delay_count;
            break;
          }
        }
        move->state = 3;
           
      case 3: // slow delay
        if( move->slow_delay )
        {
          if( move->slow_delay_count )
          {
            OC4R =  (OC4R + move->slow_delay) ;    // Initialize primary Compare Register
            OC4CONSET = _OC4CON_OCM0_MASK;
            --move->slow_delay_count;
            break;
          }
        }
        move->state = 4;
        
      case 4: // last delay
        IEC0CLR = _IEC0_OC4IE_MASK;
        if( --live_axis_count == 0 )
        {
          // this is the last axis interrupt that will occur for this move command,
          // so release it and start up the next command if there is one.
          stepper_move_buffer.releaseTail();
          if( !stepper_move_buffer.IsEmpty() )
          {
            BeginMove( stepper_move_buffer.peekTail(), OC4R );
          }
        }
        move->state = 5;
        break;
    }  
    
    IFS0CLR = _IFS0_OC4IF_MASK; // Clear the OC1 interrupt flag
  }

#else

   static bool IsFull()
   {
     return false;
   }
   
   static StepperMoves* ReserveMove()
   {
     return stepper_move_buffer.reservePush();
   }
   
   static void ReleaseMove( StepperMoves* moves )
   {
   }

#endif

};




inline void queue_wait() 
{
  for (;StepGenerator::IsFull();) 
  {
    //ifclock(clock_flag_10ms) {
    //	clock_10ms();
    //}
  }
}

inline void home_x_negative() {}
inline void home_y_negative() {}
inline void home_z_negative() {}
inline void home_x_positive() {}
inline void home_y_positive() {}
inline void home_z_positive() {}

extern PortPin debugPin;

#endif // STEP_GENERATOR_H_
