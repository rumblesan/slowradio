#ifndef __SLOW_RADIO_FILE_READER__
#define __SLOW_RADIO_FILE_READER__

#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"


typedef struct FileReaderConfig {

  int channels;

  int read_size;

  int usleep_amount;

  bstring pattern;

  RingBuffer *audio_out;

} FileReaderConfig;

FileReaderConfig *filereader_config_create(int channels,
                                           int read_size,
                                           bstring pattern,
                                           int usleep_amount,
                                           RingBuffer *audio_out);

void filereader_config_destroy(FileReaderConfig *cfg);

void *start_filereader_process(void *_cfg);

#endif
