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

FileReaderInfo *filereader_info_create(bstring name,
                                       int channels,
                                       int read_size,
                                       int usleep_amount,
                                       RingBuffer *audio_out) {

  FileReaderInfo *info = malloc(sizeof(FileReaderInfo));
  check_mem(info);

  info->name          = name;
  info->channels      = channels;
  info->read_size     = read_size;
  info->usleep_amount = usleep_amount;

  check(audio_out != NULL, "FileReader: Invalid audio out buffer passed");
  info->audio_out = audio_out;

  return info;
 error:
  return NULL;
}

void filereader_info_destroy(FileReaderInfo *info) {
  bdestroy(info->name);
  free(info);
}

void *start_filereader(void *_info) {
  FileReaderInfo *info = _info;

  SF_INFO input_info;
  SNDFILE *input_file = NULL;

  float *iob = NULL;
  Message *out_message = NULL;
  int read_amount = 0;

  check(info != NULL, "FileReader: Invalid info data passed");

  input_file = sf_open(bdata(info->name), SFM_READ, &input_info);
  check(input_file != NULL, "FileReader: Could not open input file");
  check(input_info.channels == info->channels,
        "FileReader: Only accepting files with %d channels", info->channels);

  int size = info->read_size;
  int channels = info->channels;
  int buffer_size = size * channels;

  log_info("FileReader: Starting");

  while (true) {
    if (!rb_full(info->audio_out)) {
      iob = malloc(buffer_size * sizeof(float));
      check_mem(iob);

      read_amount = sf_readf_float(input_file, iob, size);
      out_message = audio_array_message(iob, channels, read_amount);
      iob = NULL;

      check(out_message != NULL,
            "FileReader: Could not create audio array message");
      rb_push(info->audio_out, out_message);
      out_message = NULL;

      if (read_amount < size) break;
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
  log_info("FileReader: Finished");
  if (info != NULL) filereader_info_destroy(info);
  if (input_file != NULL) sf_close(input_file);
  if (iob != NULL) free(iob);
  log_info("FileReader: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
