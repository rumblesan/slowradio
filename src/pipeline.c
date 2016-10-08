#include <stdlib.h>
#include <stdbool.h>

#include "pipeline.h"

#include "bclib/ringbuffer.h"
#include "bclib/dbg.h"

Pipeline *pipeline_create(RingBuffer *input, RingBuffer *output) {
  check(input != NULL, "Invalid input RingBuffer");
  check(output != NULL, "Invalid output RingBuffer");
  Pipeline *pipeline = malloc(sizeof(Pipeline));
  check_mem(pipeline);

  pipeline->input = input;
  pipeline->output = output;

  return pipeline;
 error:
  return NULL;
}

bool pipeline_can_read(Pipeline *pipeline) {
  return !rb_empty(pipeline->input);
}

bool pipeline_can_write(Pipeline *pipeline) {
  return !rb_full(pipeline->output);
}

void *pipeline_read(Pipeline *pipeline) {
  return rb_pop(pipeline->input);
}

bool pipeline_write(Pipeline *pipeline, void *data) {
  int pos = rb_push(pipeline->output, data);
  return pos != -1;
}

void pipeline_destroy(Pipeline *pipeline) {
  check(pipeline != NULL, "Passed invalid pipeline");
  free(pipeline);
 error:
  return;
}
