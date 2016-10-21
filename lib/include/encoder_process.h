#ifndef __SLOW_RADIO_ENCODER_PROCESS__
#define __SLOW_RADIO_ENCODER_PROCESS__

#include "bclib/ringbuffer.h"

typedef enum {
  WAITINGFORFILE,
  ENCODINGFILE,
  CLOSINGSTREAM,
  ENCODERERROR,
} EncoderState;

typedef struct EncoderProcessConfig {

  RingBuffer *pipe_in;

  RingBuffer *pipe_out;

  int channels;
  int samplerate;
  int format;
  int usleep_time;
  double quality;

} EncoderProcessConfig;

EncoderProcessConfig *encoder_config_create(int channels,
                                            int samplerate,
                                            int format,
                                            double quality,
                                            int usleep_time,
                                            RingBuffer *pipe_in,
                                            RingBuffer *pipe_out);

void encoder_config_destroy(EncoderProcessConfig *cfg);

void *start_encoder(void *_cfg);

#endif
