#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "encoder_process.h"

#include "virtual_ogg.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

EncoderProcessState *encoder_process_state_create(int channels,
                                                  int samplerate,
                                                  int format,
                                                  int usleep_time,
                                                  RingBuffer *msg_in,
                                                  RingBuffer *msg_out) {

  EncoderProcessState *state = malloc(sizeof(EncoderProcessState));
  check_mem(state);

  check(msg_in != NULL, "Invalid msg in buffer passed");
  state->audio_in = msg_in;

  check(msg_out != NULL, "Invalid msg out buffer passed");
  state->audio_out = msg_out;

  state->channels    = channels;
  state->samplerate  = samplerate;
  state->format      = format;
  state->usleep_time = usleep_time;

  return state;
 error:
  return NULL;
}

void encoder_process_state_destroy(EncoderProcessState *state) {
  free(state);
}

void *start_encoder_process(void *_info) {
  EncoderProcessState *info = _info;

  SF_INFO output_info;
  SF_VIRTUAL_IO *virtual_ogg = NULL;
  SNDFILE *output_file = NULL;

  check(info != NULL, "Encoder: Invalid info data passed");

  output_info.samplerate = info->samplerate;
  output_info.channels = info->channels;
  output_info.format = info->format;

  virtual_ogg = virtual_ogg_create();
  check(virtual_ogg != NULL, "Encoder: Could not create virtual ogg");
  output_file = sf_open_virtual(virtual_ogg,
                                SFM_WRITE,
                                &output_info,
                                info->audio_out);
  check(output_file != NULL,
        "Encoder: Could not open output file: %s", sf_strerror(output_file));

  int startup_wait = 1;
  while (true) {
    if (!rb_empty(info->audio_in)) {
      log_info("Encoder: Audio available");
      break;
    } else {
      log_info("Encoder: Waiting for input audio...");
      sleep(startup_wait);
    }
  }

  Message *input_msg      = NULL;
  AudioArray *input_audio = NULL;

  log_info("Encoder: Starting");
  while (true) {
    if (!rb_full(info->audio_out) && !rb_empty(info->audio_in)) {
      input_msg = rb_pop(info->audio_in);
      check(input_msg != NULL, "Encoder: Could not get audio from audio in");
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
      usleep(info->usleep_time);
    }
  }

  while (true) {
    if (!rb_full(info->audio_out)) {
      rb_push(info->audio_out, finished_message());
      break;
    } else {
      sched_yield();
      usleep(info->usleep_time);
    }
  }

 error:
  log_info("Encoder: Finished");
  if (info != NULL) encoder_process_state_destroy(info);
  if (virtual_ogg != NULL) virtual_ogg_destroy(virtual_ogg);
  if (output_file != NULL) sf_close(output_file);
  log_info("Encoder: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
