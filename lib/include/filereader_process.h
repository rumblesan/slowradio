#ifndef __SLOW_RADIO_FILEREADER_PROCESS__
#define __SLOW_RADIO_FILEREADER_PROCESS__

#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"


typedef struct FileReaderProcessConfig {

  int channels;
  int read_size;
  bstring pattern;

  int thread_sleep;
  int max_push_msgs;
  RingBuffer *pipe_out;

} FileReaderProcessConfig;

FileReaderProcessConfig *filereader_config_create(int channels,
                                                  int read_size,
                                                  bstring pattern,
                                                  int thread_sleep,
                                                  int max_push_msgs,
                                                  RingBuffer *pipe_out);

void filereader_config_destroy(FileReaderProcessConfig *cfg);

void *start_filereader(void *_cfg);

#endif
