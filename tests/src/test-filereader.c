#include "minunit.h"
#include "filereader_process.h"

#include <bclib/ringbuffer.h> 
#include <bclib/bstrlib.h> 
#include <bclib/dbg.h> 

#include "messages.h"

char *test_filereader_config_create() {
  int channels = 2;
  int read_size = 1024;
  bstring pattern = bfromcstr("../../audio/*.ogg");
  int thread_sleep = 20;
  int max_push_msgs = 20;
  int filereader_status;

  RingBuffer *pipe_out = rb_create(100);

  FileReaderProcessConfig *s = filereader_config_create(channels,
                                                        read_size,
                                                        pattern,
                                                        -1,
                                                        thread_sleep,
                                                        max_push_msgs,
                                                        &filereader_status,
                                                        pipe_out);

  mu_assert(s != NULL, "Could not create filereader config");

  filereader_config_destroy(s);
  rb_destroy(pipe_out);
  return NULL;
}

char *test_filereader_loop() {
  int channels = 2;
  int read_size = 4096;
  bstring pattern = bfromcstr("../../audio/*.ogg");
  int thread_sleep = 20;
  int max_push_msgs = 20;
  int filereader_status = -1;

  RingBuffer *pipe_out = rb_create(100000);

  Message *output_msg = NULL;

  FileReaderProcessConfig *cfg = filereader_config_create(channels,
                                                          read_size,
                                                          pattern,
                                                          4,
                                                          thread_sleep,
                                                          max_push_msgs,
                                                          &filereader_status,
                                                          pipe_out);

  mu_assert(cfg != NULL, "Could not create filereader config");

  start_filereader(cfg);

  log_info("pipe out %d", rb_size(pipe_out));
  while (!rb_empty(pipe_out)) {
    output_msg = rb_pop(pipe_out);
    mu_assert(output_msg != NULL, "Should not receive NULL messages");
    message_destroy(output_msg);
  }

  mu_assert(filereader_status == 0, "FileReader status should be 0");
  rb_destroy(pipe_out);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_filereader_config_create);
  mu_run_test(test_filereader_loop);

  return NULL;
}

RUN_TESTS(all_tests);
