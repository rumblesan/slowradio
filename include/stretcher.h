#ifndef __SLOW_RADIO_STRETCHER__
#define __SLOW_RADIO_STRETCHER__

#include "bclib/ringbuffer.h"

typedef struct StretcherInfo {

  RingBuffer *audio_in;

  RingBuffer *audio_out;

  int window;

  float stretch;

  int channels;

} StretcherInfo;

StretcherInfo *stretcher_info_create(RingBuffer *audio_in,
                                     RingBuffer *audio_out,
                                     int window,
                                     float stretch);

void stretcher_info_destroy(StretcherInfo *info);

void *start_stretcher(void *_info);

#endif
