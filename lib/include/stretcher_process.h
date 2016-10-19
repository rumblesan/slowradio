#ifndef __SLOW_RADIO_STRETCHER_PROCESS__
#define __SLOW_RADIO_STRETCHER_PROCESS__

#include "bclib/ringbuffer.h"

typedef struct StretcherConfig {

  RingBuffer *pipe_in;

  RingBuffer *pipe_out;

  int window;

  int usleep_amount;

  float stretch;

  int channels;

} StretcherConfig;

StretcherConfig *stretcher_config_create(float stretch,
                                         int window_size,
                                         int usleep_amount,
                                         int channels,
                                         RingBuffer *pipe_in,
                                         RingBuffer *pipe_out);

void stretcher_config_destroy(StretcherConfig *cfg);

void *start_stretcher_process(void *_cfg);

#endif
