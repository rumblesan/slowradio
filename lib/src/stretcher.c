#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "stretcher.h"

#include "messages.h"

#include "pstretch/pstretch.h"
#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

StretcherInfo *stretcher_info_create(float stretch,
                                     int window_size,
                                     int usleep_amount,
                                     int channels,
                                     RingBuffer *audio_in,
                                     RingBuffer *audio_out) {

  StretcherInfo *info = malloc(sizeof(StretcherInfo));
  check_mem(info);

  check(audio_in != NULL, "Invalid audio in buffer passed");
  info->audio_in = audio_in;

  check(audio_out != NULL, "Invalid audio out buffer passed");
  info->audio_out = audio_out;

  info->window = window_size;
  info->stretch = stretch;
  info->channels = channels;
  info->usleep_amount = usleep_amount;

  return info;
 error:
  return NULL;
}

void stretcher_info_destroy(StretcherInfo *info) {
  free(info);
}

void *start_stretcher(void *_info) {
  StretcherInfo *info = _info;

  Stretch *stretch = stretch_create(info->channels, info->window, info->stretch);
  Message *input_msg = NULL;
  AudioBuffer *input_audio = NULL;
  AudioBuffer *windowed = NULL;
  AudioBuffer *stretched = NULL;
  Message *output_msg = NULL;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = info->usleep_amount;

  int startup_wait = 1;
  while (true) {
    if (!rb_empty(info->audio_in)) {
      log_info("Stretcher: Audio available");
      break;
    } else {
      log_info("Stretcher: Waiting for input audio...");
      sleep(startup_wait);
    }
  }

  log_info("Stretcher: Starting");
  while (true) {
    if (stretch->need_more_audio && !rb_empty(info->audio_in) && !rb_full(info->audio_out)) {
      input_msg = rb_pop(info->audio_in);
      check(input_msg != NULL, "Stretcher: Could not read input message");
      if (input_msg->type == STREAMFINISHED) {
        log_info("Stretcher: Stream Finished message received");
        message_destroy(input_msg);
        input_msg = NULL;
        break;
      } else if (input_msg->type == AUDIOBUFFER) {
        input_audio = input_msg->payload;
        stretch_load_samples(stretch, input_audio);
        // TODO freeing not destroying message as audio buffer
        // is destroyed inside stretch. Needs to be better
        free(input_msg);
        input_msg = NULL;
      } else if (
                 input_msg->type == NEWTRACK ||
                 input_msg->type == TRACKFINISHED
                 ) {
        log_info("Passing message through");
        rb_push(info->audio_out, input_msg);
      } else {
        log_err("Stretcher: Received invalid message of type %d", input_msg->type);
        message_destroy(input_msg);
        input_msg = NULL;
      }
    }
    if (!stretch->need_more_audio && !rb_full(info->audio_out)) {
      windowed = stretch_window(stretch);
      check(windowed != NULL, "Stretcher: Couldn't get windowed audio");
      fft_run(stretch->fft, windowed);
      stretched = stretch_output(stretch, windowed);
      check(stretched != NULL, "Stretcher: Could not get stretched audio");

      output_msg = audio_buffer_message(stretched);
      check(output_msg != NULL, "Stretcher: Output message issue");

      rb_push(info->audio_out, output_msg);
      output_msg = NULL;
      stretched = NULL;
      windowed = NULL;
    } else {
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

  while (true) {
    if (!rb_full(info->audio_out)) {
      rb_push(info->audio_out, stream_finished_message());
      break;
    } else {
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

 error:
  log_info("Stretcher: Finished");
  if (input_msg != NULL) message_destroy(input_msg);
  if (windowed != NULL) audio_buffer_destroy(windowed);
  if (stretched != NULL) audio_buffer_destroy(stretched);
  if (output_msg != NULL) message_destroy(output_msg);
  if (stretch != NULL) stretch_destroy(stretch);
  if (info != NULL) stretcher_info_destroy(info);
  log_info("Stretcher: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
