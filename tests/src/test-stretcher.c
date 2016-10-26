#include "minunit.h"
#include "stretcher_process.h"

#include <bclib/ringbuffer.h> 
#include <bclib/bstrlib.h> 
#include <bclib/dbg.h> 

#include "messages.h"

#include "pstretch/pstretch.h"

char *test_stretcher_config_create() {
  float stretch = 10.0;
  int window_size = 2048;
  int thread_sleep = 20;
  int channels = 2;
  int max_push_msgs = 10;

  RingBuffer *pipe_in = rb_create(100);
  RingBuffer *pipe_out = rb_create(100);

  StretcherProcessConfig *s = stretcher_config_create(stretch,
                                                      window_size,
                                                      thread_sleep,
                                                      channels,
                                                      max_push_msgs,
                                                      pipe_in,
                                                      pipe_out);

  mu_assert(s != NULL, "Could not create stretcher config");

  stretcher_config_destroy(s);
  rb_destroy(pipe_in);
  rb_destroy(pipe_out);
  return NULL;
}

char *test_stretcher_loop() {
  float stretch = 3.0;
  int window_size = 2048;
  int thread_sleep = 20;
  int channels = 2;
  int max_push_msgs = 10;

  int maxmsgs = 10;

  RingBuffer *pipe_in = rb_create(maxmsgs + 10);
  RingBuffer *pipe_out = rb_create(10 * maxmsgs + 10);

  AudioBuffer *audio = NULL;
  Message *input_msg = NULL;
  Message *output_msg = NULL;


  StretcherProcessConfig *s = stretcher_config_create(stretch,
                                                      window_size,
                                                      thread_sleep,
                                                      channels,
                                                      max_push_msgs,
                                                      pipe_in,
                                                      pipe_out);

  mu_assert(s != NULL, "Could not create stretcher config");

  for (int m = 0; m < maxmsgs; m += 1) {
    audio = audio_buffer_create(channels, window_size);
    input_msg = audio_buffer_message(audio);
    rb_push(pipe_in, input_msg);
  }
  input_msg = stream_finished_message();
  rb_push(pipe_in, input_msg);

  start_stretcher(s);

  mu_assert(!rb_empty(pipe_out), "Output pipe should not be empty");

  while (!rb_empty(pipe_out)) {
    output_msg = rb_pop(pipe_out);
    message_destroy(output_msg);
  }

  rb_destroy(pipe_in);
  rb_destroy(pipe_out);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_stretcher_config_create);
  mu_run_test(test_stretcher_loop);

  return NULL;
}

RUN_TESTS(all_tests);
