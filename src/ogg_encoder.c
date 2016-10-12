#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "ogg_encoder.h"

#include "virtual_ogg.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

OggEncoderInfo *ogg_encoder_info_create(RingBuffer *audio_in,
                                        RingBuffer *audio_out) {

  OggEncoderInfo *info = malloc(sizeof(OggEncoderInfo));
  check_mem(info);

  check(audio_in != NULL, "Invalid audio in buffer passed");
  info->audio_in = audio_in;

  check(audio_out != NULL, "Invalid audio out buffer passed");
  info->audio_out = audio_out;

  return info;
 error:
  return NULL;
}

void ogg_encoder_info_destroy(OggEncoderInfo *info) {
  free(info);
}

void *start_ogg_encoder(void *_info) {
  OggEncoderInfo *info = _info;

  SF_INFO output_info;
  SF_VIRTUAL_IO *virtual_ogg = NULL;
  SNDFILE *output_file = NULL;

  check(info != NULL, "Invalid info data passed");

  output_info.samplerate = 44100;
  output_info.channels = 2;
  output_info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

  virtual_ogg = virtual_ogg_create();
  check(virtual_ogg != NULL, "Could not create virtual ogg");
  output_file = sf_open_virtual(virtual_ogg,
                                SFM_WRITE,
                                &output_info,
                                info->audio_out);
  check(output_file != NULL,
        "Could not open output file: %s", sf_strerror(output_file));

  log_info("Starting ogg encoder\n");

  while (1) {
    if (!rb_empty(info->audio_in)) {
      log_info("Encoder: Audio available");
      break;
    } else {
      log_info("Encoder: Waiting for input audio...");
      sleep(2);
    }
  }

  Message *input_msg      = NULL;
  AudioArray *input_audio = NULL;
  while (true) {
    if (!rb_full(info->audio_out) && !rb_empty(info->audio_in)) {
      input_msg = rb_pop(info->audio_in);
      check(input_msg != NULL, "Could not get audio from audio in");
      if (input_msg->type == FINISHED) {
        log_info("Encoder: Finished message received");
        message_destroy(input_msg);
        break;
      } else if (input_msg->type == AUDIOARRAY) {
        input_audio = input_msg->payload;
        sf_writef_float(output_file,
                        input_audio->audio,
                        input_audio->per_channel_length);
        message_destroy(input_msg);
      } else {
        log_err("Encoder: Received invalid message of type %d", input_msg->type);
        message_destroy(input_msg);
      }
    } else {
      sched_yield();
      usleep(1000);
    }
  }

  while (true) {
    if (!rb_full(info->audio_out)) {
      rb_push(info->audio_out, finished_message());
      break;
    } else {
      sched_yield();
      usleep(10);
    }
  }

 error:
  log_info("Ogg encoder finished");
  if (info != NULL) ogg_encoder_info_destroy(info);
  if (virtual_ogg != NULL) virtual_ogg_destroy(virtual_ogg);
  if (output_file != NULL) sf_close(output_file);
  pthread_exit(NULL);
  return NULL;
}
