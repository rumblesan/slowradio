#include <stdlib.h>

#include "filechunk.h"

#include "bclib/dbg.h"


// TODO - Add Checks
FileChunk *file_chunk_create(unsigned char *data, int length) {
  FileChunk *chunk = malloc(sizeof(FileChunk));
  check_mem(chunk);

  chunk->data = data;
  chunk->length = length;

  return chunk;
 error:
  return NULL;
}

// TODO - Add Checks
void file_chunk_destroy(FileChunk *chunk) {
  free(chunk->data);
  free(chunk);
}
