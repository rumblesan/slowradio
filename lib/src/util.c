#include <unistd.h>
#include <stdbool.h>

#include "bclib/ringbuffer.h"

bool pipes_ready(RingBuffer *rb_in, RingBuffer *rb_out) {
  return (!rb_full(rb_out) && !rb_empty(rb_in));
}
