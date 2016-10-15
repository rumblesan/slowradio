#ifndef __SLOW_RADIO_ENCODER_PROCESS__
#define __SLOW_RADIO_ENCODER_PROCESS__

#include "bclib/ringbuffer.h"

typedef struct EncoderProcessState {

  RingBuffer *audio_in;

  RingBuffer *audio_out;

  int channels;
  int samplerate;
  int format;
  int usleep_time;

} EncoderProcessState;

EncoderProcessState *encoder_process_state_create(int channels,
                                                  int samplerate,
                                                  int format,
                                                  int usleep_time,
                                                  RingBuffer *audio_in,
                                                  RingBuffer *audio_out);

void encoder_process_state_destroy(EncoderProcessState *state);

void *start_encoder_process(void *_info);

#endif
