#include <stdio.h>
#include <stdlib.h>
#include <vorbis/vorbisenc.h>

#include "ogg_encoder.h"
#include "filechunk.h"

#include "bclib/dbg.h"

#define READ 1024

OggEncoderState *ogg_encoder_state(long channels, long samplerate, float quality) {
  OggEncoderState *encoder = malloc(sizeof(OggEncoderState));
  check_mem(encoder);

  vorbis_info_init(&(encoder->vi));
  vorbis_comment_init(&(encoder->vc));

  int initerr = vorbis_encode_init_vbr(&(encoder->vi), channels, samplerate, quality);
  check(!initerr, "Error initialising encoder");

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init(&(encoder->vd),&(encoder->vi));
  vorbis_block_init(&(encoder->vd),&(encoder->vb));
  return encoder;
 error:
  return NULL;
}

void set_headers(OggEncoderState *encoder) {
  vorbis_comment_add_tag(&(encoder->vc), "ENCODER", "encoder_example.c");
  vorbis_comment_add_tag(&(encoder->vc), "ARTIST", "Rumblesan");
  vorbis_comment_add_tag(&(encoder->vc), "TITLE", "More Efficient");
}

int write_headers(OggEncoderState *encoder, FileChunk *chunk) {

  ogg_packet header;
  ogg_packet header_comm;
  ogg_packet header_code;

  ogg_stream_init(&(encoder->os), rand());

  /* Vorbis streams begin with three headers; the initial header (with
     most of the codec setup parameters) which is mandated by the Ogg
     bitstream spec.  The second header holds any comment fields.  The
     third header holds the bitstream codebook.  We merely need to
     make the headers, then pass them to libvorbis one at a time;
     libvorbis handles the additional Ogg bitstream constraints */

  vorbis_analysis_headerout(&(encoder->vd), &(encoder->vc), &header, &header_comm, &header_code);

  ogg_stream_packetin(&(encoder->os), &header);
  ogg_stream_packetin(&(encoder->os), &header_comm);
  ogg_stream_packetin(&(encoder->os), &header_code);

  ogg_page output_page;
  while(1) {
    int result = ogg_stream_flush(&(encoder->os), &output_page);
    if(result == 0) break;
    file_chunk_extend(chunk, output_page.header, output_page.header_len);
    file_chunk_extend(chunk, output_page.body, output_page.body_len);
  }
  return 0;
}

int add_audio(OggEncoderState *encoder, long channels, long samplespc, float **audio) {
  if (samplespc == 0) {
    return vorbis_analysis_wrote(&(encoder->vd), 0);
  } else {
    float **buffer = vorbis_analysis_buffer(&(encoder->vd), READ);
    for (int c = 0; c < channels; c += 1) {
      memcpy(buffer[c], audio[c], samplespc * sizeof(float));
    }
    return vorbis_analysis_wrote(&(encoder->vd), samplespc);
  }
}

int write_audio(OggEncoderState *encoder, FileChunk *chunk) {
  int finished = 0;

  /* vorbis does some data preanalysis, then divvies up blocks for
     more involved (potentially parallel) processing.  Get a single
     block for encoding now */

  while(vorbis_analysis_blockout(&(encoder->vd), &(encoder->vb)) == 1){
    /* analysis, assume we want to use bitrate management */
    vorbis_analysis(&(encoder->vb),NULL);
    vorbis_bitrate_addblock(&(encoder->vb));

    ogg_packet new_packet;

    while(vorbis_bitrate_flushpacket(&(encoder->vd), &new_packet)) {

      /* weld the packet into the bitstream */
      ogg_stream_packetin(&(encoder->os), &new_packet);

      /* write out pages (if any) */
      ogg_page new_page;

      while(!finished) {
        int result = ogg_stream_pageout(&(encoder->os), &new_page);

        if(result == 0) break;
        file_chunk_extend(chunk, new_page.header, new_page.header_len);
        file_chunk_extend(chunk, new_page.body, new_page.body_len);

        /* this could be set above, but for illustrative purposes, I do
           it here (to show that vorbis does know where the stream ends) */

        if(ogg_page_eos(&new_page)) {
          log_info("Finished");
          finished = 1;
        }
      }
    }

  }

  return finished;
}

void cleanup_encoder(OggEncoderState *encoder) {
  vorbis_comment_clear(&(encoder->vc));
  ogg_stream_clear(&(encoder->os));
  vorbis_block_clear(&(encoder->vb));
  vorbis_dsp_clear(&(encoder->vd));
  vorbis_info_clear(&(encoder->vi));
  free(encoder);
}
