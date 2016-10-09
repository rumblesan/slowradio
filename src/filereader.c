#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader.h"
#include "virtual_ogg.h"

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

  RingBuffer *rb = rb_create(100);

  SF_INFO input_info;
  SNDFILE *input_file = NULL;

  SF_INFO output_info;
  SF_VIRTUAL_IO *virtual_ogg = NULL;
  SNDFILE *output_file = NULL;

  float *iob = NULL;

  input_file = sf_open(bdata(info->name),
                       SFM_READ,
                       &input_info);
  check(input_file != NULL, "Could not open input file");
  output_info.samplerate = input_info.samplerate;
  output_info.channels = 2;
  output_info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;

  virtual_ogg = virtual_ogg_create();
  check(virtual_ogg != NULL, "Could not create virtual ogg");
  output_file = sf_open_virtual(virtual_ogg,
                                SFM_WRITE,
                                &output_info,
                                rb);
  check(output_file != NULL, "Could not open output file: %s", sf_strerror(output_file));
  bool finished = false;

  int size = 2048;
  int channels = 2;
  int buffer_size = size * channels;
  int read_amount;

  iob = malloc(buffer_size * sizeof(float));
 
  printf("Starting loop\n");
  while (!finished) {
    read_amount = sf_readf_float(input_file, iob, size);
    printf("Read data %d\n", read_amount);
    if (read_amount < size) {
      printf("Finished\n");
      finished = true;
    }

    sf_writef_float(output_file, iob, read_amount);
  }

 error:
  if (input_file != NULL) sf_close(input_file);
  if (virtual_ogg != NULL) virtual_ogg_destroy(virtual_ogg);
  if (output_file != NULL) sf_close(output_file);
  if (iob != NULL) free(iob);
  pthread_exit(NULL);
  return NULL;
}
