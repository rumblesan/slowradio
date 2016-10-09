#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <sndfile.h>

#include "filereader.h"


int main (int argc, char *argv[]) {

  printf("Hello, Slow Radio\n");

  RingBuffer *rb = rb_create(100);

  bstring filename = bfromcstr("foo.ogg");
  FileReaderInfo *filereader_info = filereader_info_create(filename, rb);
  pthread_t file_thread;

  int rc1 = pthread_create(&file_thread,
                           NULL,
                           &start_filereader,
                           filereader_info);

  sleep(5);
  pthread_join(file_thread, NULL);
  return 0;
}
