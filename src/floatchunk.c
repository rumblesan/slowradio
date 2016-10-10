#include <stdlib.h>

#include "floatchunk.h"

#include "bclib/dbg.h"


// TODO - Add Checks
FloatChunk *float_chunk_create(float *data, int length) {
  FloatChunk *chunk = malloc(sizeof(FloatChunk));
  check_mem(chunk);

  chunk->data = data;
  chunk->length = length;

  return chunk;
 error:
  return NULL;
}

// TODO - Add Checks
void float_chunk_destroy(FloatChunk *chunk) {
  free(chunk->data);
  free(chunk);
}
