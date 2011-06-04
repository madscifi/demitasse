#ifndef BUFFERED_SERIAL_H_
#define BUFFERED_SERIAL_H_

#ifndef TESTBUILD
#include <plib.h>
#endif

#include "ring_buffer.h"

#ifndef TESTBUILD

    struct BufferedSerial
    {
      typedef RingBuffer<unsigned char,64> RxRingBufferType;
      typedef RingBuffer<unsigned char,256> TxRingBufferType;
  
      RxRingBufferType serial_rx_buffer;
      TxRingBufferType serial_tx_buffer;

      BufferedSerial()
      {
      }
  
      void Init( long baudRate, unsigned int pbClock )
      {
        /*
        U1MODE = UART_EN;
        U1BRG = __PIC32_pbClk / 16 / (baudRate - 1);
        U1STA = UART_RX_ENABLE | UART_TX_ENABLE;
        U1MODEbits.UARTEN	=	0x01;
        U1STAbits.UTXEN	=	0x01;

        // Configure UART1 RX Interrupt
			    //-ConfigIntUART1(UART_INT_PR2 | UART_RX_INT_EN);
        mU1ClearAllIntFlags();

        int configValue	= UART_INT_PR1 | UART_RX_INT_EN;

        SetPriorityIntU1(configValue);
        SetSubPriorityIntU1(configValue);
        mU1SetIntEnable((((configValue) >> 6) & 7)) ;
        */
    
        OpenUART1( UART_EN, UART_TX_ENABLE | UART_RX_ENABLE | UART_INT_TX_BUF_EMPTY, pbClock / 16 / (baudRate - 1) );
    
        ConfigIntUART1( 
          UART_RX_INT_EN |
          UART_INT_PR2 |
          UART_INT_SUB_PR0 |
          UART_INT_RX_CHAR /*|
          UART_TX_INT_EN*/ );
                 
      }

      void WriteCharBlocking( unsigned char c )
      {
        do
        {
          MEMORY_BARRIER();
        } while( serial_tx_buffer.IsFull() );
        serial_tx_buffer.pushWithBarrier( c );
        MEMORY_BARRIER();
        if( !mU1TXGetIntEnable() )
        {
          if( !serial_tx_buffer.IsEmpty() )
          {
            WriteUART1( serial_tx_buffer.pop() );
            MEMORY_BARRIER();
            INTEnable(INT_U1TX,INT_ENABLED);
          }
        }
      }
  
      void WriteStringBlocking( const char * str )
      {
        while( *str ) WriteCharBlocking( *str++ );
      }

      inline int GetCharNoBlocking()
      {
        int c = -1;
        MEMORY_BARRIER();
        if( !serial_rx_buffer.IsEmpty() )
        {
          c = serial_rx_buffer.popWithBarrier();
        }
        return c;
      }
  
      inline void HandleUartInterrupt()
      {
        ENABLE_INTERRUPTS();
    
        // Is this an RX interrupt?
        if( mU1RXGetIntFlag() )
        {
          unsigned char c = ReadUART1();
          if( !serial_rx_buffer.IsFull() )
          {
            serial_rx_buffer.push( c );
          }
  
          // Clear the RX interrupt Flag (must be AFTER the read)
          mU1RXClearIntFlag();
        }
  
        if ( mU1TXGetIntEnable() && mU1TXGetIntFlag() )
        {
          if( !serial_tx_buffer.IsEmpty() )
          {
            WriteUART1( serial_tx_buffer.pop() );
          }
          else
          {
            INTEnable(INT_U1TX,INT_DISABLED);
          }
          mU1TXClearIntFlag();
        }
      }
  
    };

#else

    #include <fstream>
    #include <iostream>

    struct BufferedSerial
    {
      std::ifstream inFile;

      BufferedSerial( )
      {
      }
  
      void Init( const char *inputFileName )
      {
        inFile.open( inputFileName );
      }

      void WriteCharBlocking( unsigned char c )
      {
          std::cout << c;
      }
  
      void WriteStringBlocking( const char * str )
      {
        while( *str ) WriteCharBlocking( *str++ );
      }

      inline int GetCharNoBlocking()
      {
        //if( inFile.eof() )
        //{
        //    exit(1);
        //}
        int c = inFile.get();
        return c;
      }  
    };

#endif

extern BufferedSerial gSerial;


#endif
