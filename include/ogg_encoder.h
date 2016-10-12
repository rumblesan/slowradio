#ifndef __SLOW_RADIO_OGG_ENCODER__
#define __SLOW_RADIO_OGG_ENCODER__

#include "bclib/ringbuffer.h"

typedef struct OggEncoderInfo {

  RingBuffer *audio_in;

  RingBuffer *audio_out;

  int channels;
  int samplerate;
  int format;
  int usleep_time;

} OggEncoderInfo;

OggEncoderInfo *ogg_encoder_info_create(int channels,
                                        int samplerate,
                                        int format,
                                        int usleep_time,
                                        RingBuffer *audio_in,
                                        RingBuffer *audio_out);

void ogg_encoder_info_destroy(OggEncoderInfo *info);

void *start_ogg_encoder(void *_info);

#endif
