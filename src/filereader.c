#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader.h"
#include "virtual_ogg.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

FileReaderInfo *filereader_info_create(bstring name, RingBuffer *audio_out) {

  FileReaderInfo *info = malloc(sizeof(FileReaderInfo));
  check_mem(info);

  info->name = name;

  check(audio_out != NULL, "Invalid audio out buffer passed");
  info->audio_out = audio_out;

  return info;
 error:
  return NULL;
}

void filereader_info_destroy(FileReaderInfo *info) {
  rb_destroy(info->audio_out);
  bdestroy(info->name);
  free(info);
}

void *start_filereader(void *_info) {
  FileReaderInfo *info = _info;

  SF_INFO input_info;
  SNDFILE *input_file = NULL;

  check(info != NULL, "Invalid info data passed");

  input_file = sf_open(bdata(info->name), SFM_READ, &input_info);
  check(input_file != NULL, "Could not open input file");

  int size = 2048;
  int channels = input_info.channels;
  int buffer_size = size * channels;

  log_info("Starting file reader\n");

  while (true) {
    if (!rb_full(info->audio_out)) {
      float *iob = malloc(buffer_size * sizeof(float));
      check_mem(iob);
      int read_amount = sf_readf_float(input_file, iob, size);
      Message *msg = audio_array_message(iob, channels, read_amount);
      check(msg != NULL, "Could not create audio array message");
      rb_push(info->audio_out, msg);
      if (read_amount < size) break;
    } else {
      sched_yield();
      usleep(10);
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
  log_info("File reader finished");
  if (info != NULL) filereader_info_destroy(info);
  if (input_file != NULL) sf_close(input_file);
  pthread_exit(NULL);
  return NULL;
}
