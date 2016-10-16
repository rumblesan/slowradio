#ifndef __SLOW_RADIO_ENCODER_PROCESS__
#define __SLOW_RADIO_ENCODER_PROCESS__

#include "bclib/ringbuffer.h"

typedef enum {
  WAITINGFORFILE,
  ENCODINGFILE,
  CLOSINGSTREAM,
  ENCODERERROR,
} EncoderState;

typedef struct EncoderProcessState {

  RingBuffer *pipe_in;

  RingBuffer *pipe_out;

  int channels;
  int samplerate;
  int format;
  int usleep_time;

} EncoderProcessState;

EncoderProcessState *encoder_process_state_create(int channels,
                                                  int samplerate,
                                                  int format,
                                                  int usleep_time,
                                                  RingBuffer *pipe_in,
                                                  RingBuffer *pipe_out);

void encoder_process_state_destroy(EncoderProcessState *state);

void *start_encoder_process(void *_info);

#endif
