#ifndef __SLOW_RADIO_STRETCHER_PROCESS__
#define __SLOW_RADIO_STRETCHER_PROCESS__

#include "bclib/ringbuffer.h"

typedef struct StretcherProcessConfig {

  RingBuffer *pipe_in;

  RingBuffer *pipe_out;

  int window;

  int usleep_amount;

  float stretch;

  int channels;

} StretcherProcessConfig;

StretcherProcessConfig *stretcher_config_create(float stretch,
                                         int window_size,
                                         int usleep_amount,
                                         int channels,
                                         RingBuffer *pipe_in,
                                         RingBuffer *pipe_out);

void stretcher_config_destroy(StretcherProcessConfig *cfg);

void *start_stretcher(void *_cfg);

#endif
