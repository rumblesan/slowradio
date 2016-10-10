#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader.h"
#include "filechunk.h"
#include "shoutcast.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"


int main (int argc, char *argv[]) {

  printf("Hello, Slow Radio\n");

  RingBuffer *rb = rb_create(100);

  bstring filename = bfromcstr("foo.ogg");
  FileReaderInfo *filereader_info = filereader_info_create(filename, rb);
  pthread_t file_thread;

  ShoutCastInfo *sc_info =
    shoutcast_info_create(bfromcstr("127.0.0.1"),
                          9090,
                          bfromcstr("source"),
                          bfromcstr("password"),
                          bfromcstr("/test.ogg"),
                          rb);

  int rc1 = pthread_create(&file_thread,
                           NULL,
                           &start_filereader,
                           filereader_info);

  pthread_t shout_thread;
  int rc2 = pthread_create(&shout_thread,
                           NULL,
                           &start_shoutcast,
                           sc_info);

  pthread_join(file_thread, NULL);
  pthread_join(shout_thread, NULL);
  return 0;
}
