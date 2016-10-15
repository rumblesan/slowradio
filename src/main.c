#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader.h"
#include "stretcher.h"
#include "encoder_process.h"
#include "shoutcast.h"
#include "config.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"


int main (int argc, char *argv[]) {

  log_info("Hello, Slow Radio");


  char *config_path = argv[1];

  RadioConfig *radio_config = read_config(config_path);

  RingBuffer *fread2stretch = rb_create(100);
  RingBuffer *stretch2encode = rb_create(100);
  RingBuffer *encode2stream = rb_create(100);

  FileReaderInfo *filereader_info =
    filereader_info_create(radio_config->channels,
                           radio_config->filereader.read_size,
                           radio_config->filereader.pattern,
                           radio_config->filereader.usleep_time,
                           fread2stretch);

  StretcherInfo *stretcher_info =
    stretcher_info_create(radio_config->stretcher.stretch,
                          radio_config->stretcher.window_size,
                          radio_config->stretcher.usleep_time,
                          radio_config->channels,
                          fread2stretch,
                          stretch2encode);

  EncoderProcessState *encoder_process_state =
    encoder_process_state_create(radio_config->channels,
                                 radio_config->encoder.samplerate,
                                 SF_FORMAT_OGG | SF_FORMAT_VORBIS,
                                 radio_config->encoder.usleep_time,
                                 stretch2encode, encode2stream);

  ShoutCastInfo *sc_info =
    shoutcast_info_create(radio_config->shoutcast.host,
                          radio_config->shoutcast.port,
                          radio_config->shoutcast.source,
                          radio_config->shoutcast.password,
                          radio_config->shoutcast.mount,
                          SHOUT_PROTOCOL_HTTP,
                          SHOUT_FORMAT_OGG,
                          encode2stream);

  pthread_t reader_thread = NULL;
  int rc1 = pthread_create(&reader_thread,
                           NULL,
                           &start_filereader,
                           filereader_info);
  check(!rc1, "Error creating File Reader thread");

  pthread_t stretcher_thread = NULL;
  int rc2 = pthread_create(&stretcher_thread,
                           NULL,
                           &start_stretcher,
                           stretcher_info);
  check(!rc2, "Error creating File Reader thread");

  pthread_t encoder_thread = NULL;
  int rc3 = pthread_create(&encoder_thread,
                           NULL,
                           &start_encoder_process,
                           encoder_process_state);
  check(!rc3, "Error creating File Reader thread");

  pthread_t shout_thread = NULL;
  int rc4 = pthread_create(&shout_thread,
                           NULL,
                           &start_shoutcast,
                           sc_info);
  check(!rc4, "Error creating File Reader thread");

  pthread_join(reader_thread, NULL);
  pthread_join(stretcher_thread, NULL);
  pthread_join(encoder_thread, NULL);
  pthread_join(shout_thread, NULL);
  return 0;
 error:
  return 1;
}
