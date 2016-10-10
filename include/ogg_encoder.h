#ifndef __SLOW_RADIO_OGG_ENCODER__
#define __SLOW_RADIO_OGG_ENCODER__

#include "bclib/ringbuffer.h"

typedef struct OggEncoderInfo {

  RingBuffer *audio_in;

  RingBuffer *audio_out;

} OggEncoderInfo;

OggEncoderInfo *ogg_encoder_info_create(RingBuffer *audio_in,
                                        RingBuffer *audio_out);

void ogg_encoder_info_destroy(OggEncoderInfo *info);

void *start_ogg_encoder(void *_info);

#endif
