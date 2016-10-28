#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "stretcher_process.h"

#include "messages.h"
#include "logging.h"

#include "pstretch/pstretch.h"
#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

StretcherProcessConfig *stretcher_config_create(int channels,
                                                int window_size,
                                                float stretch,
                                                int thread_sleep,
                                                int max_push_msgs,
                                                RingBuffer *pipe_in,
                                                RingBuffer *pipe_out) {

  StretcherProcessConfig *cfg = malloc(sizeof(StretcherProcessConfig));
  check_mem(cfg);

  check(pipe_in != NULL, "Invalid pipe in buffer passed");
  cfg->pipe_in = pipe_in;

  check(pipe_out != NULL, "Invalid pipe out buffer passed");
  cfg->pipe_out = pipe_out;

  cfg->channels     = channels;
  cfg->window       = window_size;
  cfg->stretch      = stretch;

  cfg->thread_sleep  = thread_sleep;
  cfg->max_push_msgs = max_push_msgs;

  return cfg;
 error:
  return NULL;
}

void stretcher_config_destroy(StretcherProcessConfig *cfg) {
  free(cfg);
}

void *start_stretcher(void *_cfg) {
  StretcherProcessConfig *cfg = _cfg;

  Stretch *stretch = stretch_create(cfg->channels, cfg->window, cfg->stretch);
  Message *input_msg = NULL;
  AudioBuffer *input_audio = NULL;
  AudioBuffer *windowed = NULL;
  AudioBuffer *stretched = NULL;
  Message *output_msg = NULL;
  int pushed_msgs = 0;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = cfg->thread_sleep;

  logger("Stretcher", "Starting");
  while (true) {
    if (stretch->need_more_audio && !rb_empty(cfg->pipe_in) && !rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      input_msg = rb_pop(cfg->pipe_in);
      check(input_msg != NULL, "Stretcher: Could not read input message");

      if (input_msg->type == STREAMFINISHED) {
        logger("Stretcher", "Stream Finished message received");
        message_destroy(input_msg);
        input_msg = NULL;
        break;
      } else if (input_msg->type == AUDIOBUFFER) {
        input_audio = input_msg->payload;
        stretch_load_samples(stretch, input_audio);
        message_destroy(input_msg);
        input_msg = NULL;
      } else if (
                 input_msg->type == NEWTRACK ||
                 input_msg->type == TRACKFINISHED
                 ) {
        logger("Stretcher", "Passing message through");
        rb_push(cfg->pipe_out, input_msg);
      } else {
        err_logger("Stretcher", "Received invalid message of type %d", input_msg->type);
        message_destroy(input_msg);
        input_msg = NULL;
      }
    }
    if (!stretch->need_more_audio && !rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      windowed = stretch_window(stretch);
      check(windowed != NULL, "Stretcher: Couldn't get windowed audio");
      fft_run(stretch->fft, windowed);
      stretched = stretch_output(stretch, windowed);
      check(stretched != NULL, "Stretcher: Could not get stretched audio");

      output_msg = audio_buffer_message(stretched);
      check(output_msg != NULL, "Stretcher: Output message issue");

      rb_push(cfg->pipe_out, output_msg);
      output_msg = NULL;
      stretched = NULL;
      windowed = NULL;
    } else {
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

 error:
  logger("Stretcher", "Finished");
  if (input_msg != NULL) message_destroy(input_msg);
  if (windowed != NULL) audio_buffer_destroy(windowed);
  if (stretched != NULL) audio_buffer_destroy(stretched);
  if (output_msg != NULL) message_destroy(output_msg);
  if (stretch != NULL) stretch_destroy(stretch);
  if (cfg != NULL) stretcher_config_destroy(cfg);
  logger("Stretcher", "Cleaned up");
  return NULL;
}
