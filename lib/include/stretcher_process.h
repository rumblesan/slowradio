#ifndef __SLOW_RADIO_STRETCHER_PROCESS__
#define __SLOW_RADIO_STRETCHER_PROCESS__

#include "bclib/ringbuffer.h"

typedef struct StretcherProcessConfig {

  int channels;
  int window;
  float stretch;

  int thread_sleep;
  int max_push_msgs;
  RingBuffer *pipe_in;
  RingBuffer *pipe_out;

} StretcherProcessConfig;

StretcherProcessConfig *stretcher_config_create(int channels,
                                                int window_size,
                                                float stretch,
                                                int thread_sleep,
                                                int max_push_msgs,
                                                RingBuffer *pipe_in,
                                                RingBuffer *pipe_out);

void stretcher_config_destroy(StretcherProcessConfig *cfg);

void *start_stretcher(void *_cfg);

#endif
