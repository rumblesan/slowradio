#ifndef __SLOW_RADIO_FLOAT_CHUNK__
#define __SLOW_RADIO_FLOAT_CHUNK__

typedef struct FloatChunk {
  float *data;
  int length;
} FloatChunk;

FloatChunk *float_chunk_create(float *data, int length);

void float_chunk_destroy(FloatChunk *chunk);

#endif
