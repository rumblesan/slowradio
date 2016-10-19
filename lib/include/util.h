#ifndef __SLOW_RADIO_UTIL__
#define __SLOW_RADIO_UTIL__

#include <stdbool.h>

#include "bclib/ringbuffer.h"

bool wait_for_input(RingBuffer *rb, long pause, long maxtime);

bool pipes_ready(RingBuffer *rb_in, RingBuffer *rb_out);

#endif
