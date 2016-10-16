#ifndef __SLOW_RADIO_OGG_ENCODER__
#define __SLOW_RADIO_OGG_ENCODER__

#include <vorbis/vorbisenc.h>

#include "pstretch/audiobuffer.h"
#include "filechunk.h"

typedef struct OggEncoderState {

  ogg_stream_state  os;

  vorbis_comment    vc;

  vorbis_info       vi;

  vorbis_dsp_state  vd;
  vorbis_block      vb;

} OggEncoderState;

OggEncoderState *ogg_encoder_state(long channels, long samplerate, float quality);

void cleanup_encoder(OggEncoderState *encoder);

void set_headers(OggEncoderState *encoder);

int write_headers(OggEncoderState *encoder, FileChunk *chunk);

int add_audio(OggEncoderState *encoder, AudioBuffer *audio);

int file_finished(OggEncoderState *encoder);

int write_audio(OggEncoderState *encoder, FileChunk *chunk);

#endif
