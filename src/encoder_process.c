#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "encoder_process.h"

#include "ogg_encoder.h"
#include "filechunk.h"
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

  Message *input_msg      = NULL;
  AudioArray *input_audio = NULL;
  OggEncoderState *encoder = NULL;
  FileChunk *audio_data    = NULL;
  Message *audio_msg       = NULL;

  check(info != NULL, "Encoder: Invalid info data passed");

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

  float quality = 0.5;
  encoder = ogg_encoder_state(info->channels, info->samplerate, quality);
  set_headers(encoder);
  FileChunk *headers = file_chunk_create();
  check(headers != NULL, "Could not create headers file chunk");
  write_headers(encoder, headers);
  Message *header_msg = file_chunk_message(headers);
  check(header_msg != NULL, "Could not create headers message");

  rb_push(info->audio_out, header_msg);

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
        add_audio(encoder,
                  input_audio->channels,
                  input_audio->per_channel_length,
                  input_audio->audio);

        audio_data = file_chunk_create();
        write_audio(encoder, audio_data);
        if (audio_data->data == NULL) {
          free(audio_data);
        } else {
          check(audio_data->data != NULL, "Not extended audio data");
          audio_msg = file_chunk_message(audio_data);
          check(audio_msg != NULL, "Could not create audio message");
          rb_push(info->audio_out, audio_msg);
        }

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

  log_info("Encoder: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
