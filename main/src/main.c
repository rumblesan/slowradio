#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sndfile.h>

#include "filereader_process.h"
#include "stretcher_process.h"
#include "encoder_process.h"
#include "broadcast_process.h"
#include "config.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"


int main (int argc, char *argv[]) {

  log_info("Hello, Slow Radio");


  char *config_path = argv[1];

  RadioInputCfg *radio_config = read_config(config_path);

  RingBuffer *fread2stretch = rb_create(100);
  RingBuffer *stretch2encode = rb_create(100);
  RingBuffer *encode2stream = rb_create(100);

  FileReaderProcessConfig *filereader_cfg =
    filereader_config_create(radio_config->channels,
                             radio_config->filereader.read_size,
                             radio_config->filereader.pattern,
                             radio_config->filereader.usleep_time,
                             fread2stretch);

  StretcherProcessConfig *stretcher_cfg =
    stretcher_config_create(radio_config->stretcher.stretch,
                            radio_config->stretcher.window_size,
                            radio_config->stretcher.usleep_time,
                            radio_config->channels,
                            fread2stretch,
                            stretch2encode);

  EncoderProcessConfig *encoder_cfg =
    encoder_config_create(radio_config->channels,
                                 radio_config->encoder.samplerate,
                                 SF_FORMAT_OGG | SF_FORMAT_VORBIS,
                                 radio_config->encoder.quality,
                                 radio_config->encoder.usleep_time,
                                 stretch2encode, encode2stream);

  BroadcastProcessConfig *broadcast_cfg =
    broadcast_config_create(radio_config->shoutcast.host,
                            radio_config->shoutcast.port,
                            radio_config->shoutcast.source,
                            radio_config->shoutcast.password,
                            radio_config->shoutcast.mount,
                            radio_config->shoutcast.name,
                            radio_config->shoutcast.description,
                            radio_config->shoutcast.genre,
                            radio_config->shoutcast.url,
                            SHOUT_PROTOCOL_HTTP,
                            SHOUT_FORMAT_OGG,
                            encode2stream);

  pthread_t reader_thread;
  int rc1 = pthread_create(&reader_thread,
                           NULL,
                           &start_filereader,
                           filereader_cfg);
  check(!rc1, "Error creating File Reader thread");

  pthread_t stretcher_thread;
  int rc2 = pthread_create(&stretcher_thread,
                           NULL,
                           &start_stretcher,
                           stretcher_cfg);
  check(!rc2, "Error creating File Reader thread");

  pthread_t encoder_thread;
  int rc3 = pthread_create(&encoder_thread,
                           NULL,
                           &start_encoder,
                           encoder_cfg);
  check(!rc3, "Error creating File Reader thread");

  pthread_t broadcast_thread;
  int rc4 = pthread_create(&broadcast_thread,
                           NULL,
                           &start_broadcast,
                           broadcast_cfg);
  check(!rc4, "Error creating Broadcasting thread");

  pthread_join(reader_thread, NULL);
  pthread_join(stretcher_thread, NULL);
  pthread_join(encoder_thread, NULL);
  pthread_join(broadcast_thread, NULL);
  return 0;
 error:
  return 1;
}
