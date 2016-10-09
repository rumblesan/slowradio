#ifndef __SLOW_RADIO_FILE_READER__
#define __SLOW_RADIO_FILE_READER__

#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"


typedef struct FileReaderInfo {

  bstring name;
  
  RingBuffer *audio_out;

} FileReaderInfo;

FileReaderInfo *filereader_info_create(bstring name, RingBuffer *audio_out);

void filereader_info_destroy(FileReaderInfo *info);

void *start_filereader(void *_info);

#endif
