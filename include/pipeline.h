#ifndef __SLOW_RADIO_PIPELINE__
#define __SLOW_RADIO_PIPELINE__

#include <stdbool.h>

#include "bclib/ringbuffer.h"

typedef struct Pipeline {

  RingBuffer *input;
  RingBuffer *output;

} Pipeline;

Pipeline *pipeline_create(RingBuffer *input, RingBuffer *output);

bool pipeline_can_read(Pipeline *pipeline);

bool pipeline_can_write(Pipeline *pipeline);

void *pipeline_read(Pipeline *pipeline);

bool pipeline_write(Pipeline *pipeline, void *data);

void pipeline_destroy(Pipeline *pipeline);

#endif
