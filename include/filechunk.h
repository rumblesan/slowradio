#ifndef __SLOW_RADIO_FILE_CHUNK__
#define __SLOW_RADIO_FILE_CHUNK__

typedef struct FileChunk {
  unsigned char *data;
  int length;
} FileChunk;

FileChunk *file_chunk_create(unsigned char *data, int length);

void file_chunk_destroy(FileChunk *chunk);

#endif
