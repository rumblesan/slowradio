#include <stdlib.h>

#include "filechunk.h"
#include "bclib/dbg.h"

FileChunk *file_chunk_create() {
  FileChunk *chunk = malloc(sizeof(FileChunk));
  check_mem(chunk);
  chunk->data = NULL;
  chunk->length = 0;
  return chunk;
 error:
  return NULL;
}

FileChunk *file_chunk_extend(FileChunk *chunk, unsigned char *addition, int addlength) {
  if (chunk->length == 0) {
    chunk->data = malloc(addlength * sizeof(unsigned char));
    check_mem(chunk->data);

    memcpy(chunk->data, addition, addlength * sizeof(unsigned char));
    chunk->length = addlength;
  } else {
    int old_length = chunk->length;
    int new_length = old_length + addlength;
    chunk->data = realloc(chunk->data, new_length * sizeof(unsigned char));
    check_mem(chunk->data);

    memcpy(chunk->data[old_length], addition, addlength * sizeof(unsigned char));
    chunk->length = new_length;
  }
  return chunk;
 error:
  return NULL;
}

void file_chunk_destroy(FileChunk *chunk) {
  check(chunk != NULL, "Invalid file chunk");
  check(chunk->data != NULL, "Invalid data in file chunk");
  free(chunk->data);
  free(chunk);
 error:
  return;
}
