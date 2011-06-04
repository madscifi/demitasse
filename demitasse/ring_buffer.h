#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include "processor.h"

template<class element_type, unsigned int len>
class RingBuffer
{
public:  
  RingBuffer()
  : head(0),
    tail(0)
  {
  }
  
  bool IsEmpty()
  {
	return head == tail;
  }
  
  bool IsFull()
  {
    return ((head + 1) % len ) == tail;
  }

  void push( const element_type e )
  {
    unsigned int h = head;
    buffer[ h ] = e;
    h = (h + 1) % (len);
    head = h;
  }

  void pushWithBarrier( const element_type e )
  {
    unsigned int h = head;
    buffer[ h ] = e;
    h = (h + 1) % (len);
    MEMORY_BARRIER();
    head = h;
  }

  element_type pop()
  {
    unsigned int t = tail;
    element_type e = buffer[t];
    t = (t+1)%(len);
    tail = t;
    return e;
  }

  element_type popWithBarrier()
  {
    unsigned int t = tail;
    element_type e = buffer[t];
    t = (t+1)%(len);
    MEMORY_BARRIER();
    tail = t;
    return e;
  }

  element_type* reservePush()
  {
    unsigned int h = head;
    return &buffer[ h ];
  }

  void releasePush()
  {
    head = (head + 1) % (len);
  }

  element_type* peekTail()
  {
    return &buffer[ tail ];
  }

  void releaseTail()
  {
    tail = (tail + 1) % len;
  }
  
private:
  unsigned int head;
  unsigned int tail;
  element_type buffer[ len ];

};

#endif

