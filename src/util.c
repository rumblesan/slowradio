#include <unistd.h>
#include <stdbool.h>

#include "bclib/ringbuffer.h"


bool wait_for_input(RingBuffer *rb, long pause, long maxtime) {
  int maxloops = maxtime / pause;
  int loops    = 0;
  while (true) {
    if (!rb_empty(rb)) {
      break;
    } else {
      sleep(pause);
    }
    loops += 1;
    if (loops >= maxloops) return false;
  }
  return true;
}
