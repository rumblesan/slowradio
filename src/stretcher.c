#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "stretcher.h"

#include "floatchunk.h"

#include "pstretch/pstretch.h"
#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

StretcherInfo *stretcher_info_create(RingBuffer *audio_in,
                                     RingBuffer *audio_out,
                                     int window,
                                     float stretch) {

  StretcherInfo *info = malloc(sizeof(StretcherInfo));
  check_mem(info);

  check(audio_in != NULL, "Invalid audio in buffer passed");
  info->audio_in = audio_in;

  check(audio_out != NULL, "Invalid audio out buffer passed");
  info->audio_out = audio_out;

  info->window = window;
  info->stretch = stretch;
  info->channels = 2;

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
  FloatChunk *next_input = NULL;
  AudioBuffer *windowed = NULL;
  AudioBuffer *stretched = NULL;
  float *floats = NULL;
  FloatChunk *output_audio = NULL;

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
      next_input = rb_pop(info->audio_in);
      check(next_input != NULL,
            "Stretcher: Could not read next input from audio in");
      stretch_load_floats(stretch,
                          next_input->data,
                          info->channels,
                          next_input->length);
      float_chunk_destroy(next_input);
    }
    if (!stretch->need_more_audio && !rb_full(info->audio_out)) {
      windowed = stretch_window(stretch);
      check(windowed != NULL, "Stretcher: Couldn't get windowed audio");
      fft_run(stretch->fft, windowed);
      stretched = stretch_output(stretch, windowed);
      check(stretched != NULL, "Stretcher: Could not get stretched audio");

      int buffer_size = stretched->channels * stretched->size;
      floats = malloc(buffer_size * sizeof(float)); /*  */
      check_mem(floats);
      for (int i = 0; i < stretched->channels; i++) {
        for (int j = 0; j < stretched->size; j++) {
          int pos = (j * stretched->channels) + i;
          floats[pos] = stretched->buffers[i][j];
        }
      }

      output_audio = float_chunk_create(floats, buffer_size);
      check(output_audio != NULL, "Stretcher: Output audio issue");
      floats = NULL;
      rb_push(info->audio_out, output_audio);
      audio_buffer_destroy(stretched);
    } else {
      sched_yield();
      usleep(1000);
    }
  }

 error:
  log_info("Stretcher: Finished");
  if (next_input != NULL) float_chunk_destroy(next_input);
  if (windowed != NULL) audio_buffer_destroy(windowed);
  if (stretched != NULL) audio_buffer_destroy(stretched);
  if (floats != NULL) free(floats);
  if (output_audio != NULL) float_chunk_destroy(output_audio);
  if (stretch != NULL) stretch_destroy(stretch);
  if (stream != NULL) audio_stream_destroy(stream);
  if (info != NULL) stretcher_info_destroy(info);
  return NULL;
}
