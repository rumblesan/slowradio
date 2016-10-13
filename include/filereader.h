#ifndef __SLOW_RADIO_FILE_READER__
#define __SLOW_RADIO_FILE_READER__

#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"


typedef struct FileReaderInfo {

  int channels;

  int read_size;

  int usleep_amount;

  bstring pattern;

  RingBuffer *audio_out;

} FileReaderInfo;

FileReaderInfo *filereader_info_create(int channels,
                                       int read_size,
                                       bstring pattern,
                                       int usleep_amount,
                                       RingBuffer *audio_out);

void filereader_info_destroy(FileReaderInfo *info);

void *start_filereader(void *_info);

#endif
