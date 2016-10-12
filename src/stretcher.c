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

AudioBuffer *audio_file_stream_reader(AudioStream *stream,
                                      int sample_count) {
  return NULL;
}

void *start_stretcher(void *_info) {
  StretcherInfo *info = _info;

  AudioStream *stream = audio_stream_create(info->audio_in);

  debug("window: %d  stretch: %f  channels: %d", info->window, info->stretch, info->channels);
  Stretch *stretch = stretch_create(info->channels,
                                    info->window,
                                    info->stretch,
                                    &audio_file_stream_reader,
                                    stream 
                                    );
  Message *input_msg = NULL;
  AudioArray *input_audio = NULL;
  AudioBuffer *windowed = NULL;
  AudioBuffer *stretched = NULL;
  float *floats = NULL;
  Message *output_audio = NULL;

  while (1) {
    if (!rb_empty(info->audio_in)) {
      log_info("Stretcher: Audio available");
      break;
    } else {
      log_info("Stretcher: Waiting for input audio...");
      sleep(2);
    }
  }

  log_info("Stretcher: Starting");
  while (true) {
    if (stretch->need_more_audio && !rb_empty(info->audio_in)) {
      input_msg = rb_pop(info->audio_in);
      check(input_msg != NULL, "Stretcher: Could not read input message");
      if (input_msg->type == FINISHED) {
        log_info("Stretcher: Finished message received");
        message_destroy(input_msg);
        input_msg = NULL;
        break;
      } else if (input_msg->type == AUDIOARRAY) {
        input_audio = input_msg->payload;
        stretch_load_floats(stretch,
                            input_audio->audio,
                            input_audio->channels,
                            input_audio->per_channel_length);
        message_destroy(input_msg);
        input_msg = NULL;
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
      windowed = NULL;

      int buffer_size = stretched->channels * stretched->size;
      floats = malloc(buffer_size * sizeof(float)); /*  */
      check_mem(floats);
      for (int i = 0; i < stretched->channels; i++) {
        for (int j = 0; j < stretched->size; j++) {
          int pos = (j * stretched->channels) + i;
          floats[pos] = stretched->buffers[i][j];
        }
      }
      output_audio = audio_array_message(floats, stretched->channels, buffer_size);
      check(output_audio != NULL, "Stretcher: Output message issue");

      audio_buffer_destroy(stretched);
      stretched = NULL;

      floats = NULL;
      rb_push(info->audio_out, output_audio);
      output_audio = NULL;
    } else {
      sched_yield();
      usleep(info->usleep_amount);
    }
  }

  while (true) {
    if (!rb_full(info->audio_out)) {
      rb_push(info->audio_out, finished_message());
      break;
    } else {
      sched_yield();
      usleep(info->usleep_amount);
    }
  }

 error:
  log_info("Stretcher: Finished");
  if (input_msg != NULL) message_destroy(input_msg);
  if (windowed != NULL) audio_buffer_destroy(windowed);
  if (stretched != NULL) audio_buffer_destroy(stretched);
  if (floats != NULL) free(floats);
  if (output_audio != NULL) message_destroy(output_audio);
  if (stretch != NULL) stretch_destroy(stretch);
  if (stream != NULL) audio_stream_destroy(stream);
  if (info != NULL) stretcher_info_destroy(info);
  pthread_exit(NULL);
  return NULL;
}
