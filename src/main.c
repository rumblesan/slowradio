#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader.h"
#include "stretcher.h"
#include "ogg_encoder.h"
#include "shoutcast.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"


int main (int argc, char *argv[]) {

  printf("Hello, Slow Radio\n");

  RingBuffer *fread2stretch = rb_create(100);
  RingBuffer *stretch2encode = rb_create(100);
  RingBuffer *encode2stream = rb_create(100);

  FileReaderInfo *filereader_info =
    filereader_info_create(bfromcstr(argv[1]), fread2stretch);

  StretcherInfo *stretcher_info =
    stretcher_info_create(fread2stretch,
                          stretch2encode,
                          4096, 20);

  OggEncoderInfo *ogg_encoder_info =
    ogg_encoder_info_create(stretch2encode, encode2stream);

  ShoutCastInfo *sc_info =
    shoutcast_info_create(bfromcstr("127.0.0.1"),
                          9090,
                          bfromcstr("source"),
                          bfromcstr("password"),
                          bfromcstr("/test.ogg"),
                          encode2stream);

  pthread_t reader_thread;
  int rc1 = pthread_create(&reader_thread,
                           NULL,
                           &start_filereader,
                           filereader_info);

  pthread_t stretcher_thread;
  int rc2 = pthread_create(&stretcher_thread,
                           NULL,
                           &start_stretcher,
                           stretcher_info);

  pthread_t encoder_thread;
  int rc3 = pthread_create(&encoder_thread,
                           NULL,
                           &start_ogg_encoder,
                           ogg_encoder_info);

  pthread_t shout_thread;
  int rc4 = pthread_create(&shout_thread,
                           NULL,
                           &start_shoutcast,
                           sc_info);

  pthread_join(reader_thread, NULL);
  pthread_join(encoder_thread, NULL);
  pthread_join(shout_thread, NULL);
  return 0;
}
