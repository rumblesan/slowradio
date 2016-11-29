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
#include "logging.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"

int cleanup_pipe(RingBuffer *pipe, const char *pipename) {
  Message *pipe_msg = NULL;
  check(pipe != NULL, "Invalid pipe passed in");
  while (!rb_empty(pipe)) {
    pipe_msg = rb_pop(pipe);
    check(pipe_msg != NULL, "Null message whilst emptying %s pipe", pipename);
    message_destroy(pipe_msg);
  }
  rb_destroy(pipe);
  return 0;
 error:
  return 1;
}

int main (int argc, char *argv[]) {

  RadioInputCfg *radio_config = NULL;

  RingBuffer *fread2stretch = NULL;
  RingBuffer *stretch2encode = NULL;
  RingBuffer *encode2broadcast = NULL;

  FileReaderProcessConfig *filereader_cfg = NULL;
  StretcherProcessConfig *stretcher_cfg = NULL;
  EncoderProcessConfig *encoder_cfg = NULL;
  BroadcastProcessConfig *broadcast_cfg = NULL;

  pthread_t reader_thread;
  pthread_attr_t reader_thread_attr;
  pthread_t stretcher_thread;
  pthread_attr_t stretcher_thread_attr;
  pthread_t encoder_thread;
  pthread_attr_t encoder_thread_attr;
  pthread_t broadcast_thread;
  pthread_attr_t broadcast_thread_attr;
  int broadcast_status = 0;

  startup_log("SlowRadio", "Hello, Slow Radio");

  check(argc == 2, "Need to give config file path argument");

  char *config_path = argv[1];

  radio_config = read_config(config_path);
  check(radio_config != NULL, "Could not read config file");

  fread2stretch = rb_create(100);
  check(fread2stretch != NULL, "Couldn't create coms from filereader to stretcher");
  stretch2encode = rb_create(100);
  check(stretch2encode != NULL, "Couldn't create coms from stretcher to encoder");
  encode2broadcast = rb_create(100);
  check(encode2broadcast != NULL, "Couldn't create coms from encoder to broadcaster");

  int max_push_msgs = 10;

  filereader_cfg = filereader_config_create(radio_config->channels,
                                            radio_config->filereader.read_size,
                                            radio_config->filereader.pattern,
                                            -1,
                                            radio_config->filereader.thread_sleep,
                                            max_push_msgs,
                                            fread2stretch);
  check(filereader_cfg != NULL, "Couldn't create file reader process config");

  stretcher_cfg = stretcher_config_create(radio_config->channels,
                                          radio_config->stretcher.window_size,
                                          radio_config->stretcher.stretch,
                                          radio_config->stretcher.thread_sleep,
                                          max_push_msgs,
                                          fread2stretch,
                                          stretch2encode);
  check(stretcher_cfg != NULL, "Couldn't create stretcher process config");

  encoder_cfg = encoder_config_create(radio_config->channels,
                                      radio_config->encoder.samplerate,
                                      SF_FORMAT_OGG | SF_FORMAT_VORBIS,
                                      radio_config->encoder.quality,
                                      radio_config->encoder.thread_sleep,
                                      max_push_msgs,
                                      stretch2encode, encode2broadcast);
  check(encoder_cfg != NULL, "Couldn't create encoder process config");

  broadcast_cfg = broadcast_config_create(radio_config->broadcast.host,
                                          radio_config->broadcast.port,
                                          radio_config->broadcast.source,
                                          radio_config->broadcast.password,
                                          radio_config->broadcast.mount,
                                          radio_config->broadcast.name,
                                          radio_config->broadcast.description,
                                          radio_config->broadcast.genre,
                                          radio_config->broadcast.url,
                                          SHOUT_PROTOCOL_HTTP,
                                          SHOUT_FORMAT_OGG,
                                          &broadcast_status,
                                          encode2broadcast);
  check(broadcast_cfg != NULL, "Couldn't create broadcast process config");


  check(!pthread_attr_init(&reader_thread_attr),
        "Error setting reader thread attributes");
  check(!pthread_attr_setdetachstate(&reader_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting reader thread detach state");
  check(!pthread_create(&reader_thread,
                        &reader_thread_attr,
                        &start_filereader,
                        filereader_cfg),
        "Error creating file reader thread");

  check(!pthread_attr_init(&stretcher_thread_attr),
        "Error setting stretcher thread attributes");
  check(!pthread_attr_setdetachstate(&stretcher_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting stretcher thread detach state");
  check(!pthread_create(&stretcher_thread,
                        &stretcher_thread_attr,
                        &start_stretcher,
                        stretcher_cfg),
        "Error creating stretcher thread");

  check(!pthread_attr_init(&encoder_thread_attr),
        "Error setting encoder thread attributes");
  check(!pthread_attr_setdetachstate(&encoder_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting encoder thread detach state");
  check(!pthread_create(&encoder_thread,
                        &encoder_thread_attr,
                        &start_encoder,
                        encoder_cfg),
        "Error creating encoder thread");

  check(!pthread_attr_init(&broadcast_thread_attr),
        "Error setting broadcast thread attributes");
  check(!pthread_attr_setdetachstate(&broadcast_thread_attr, PTHREAD_CREATE_DETACHED),
        "Error setting broadcast thread detach state");
  check(!pthread_create(&broadcast_thread,
                        &broadcast_thread_attr,
                        &start_broadcast,
                        broadcast_cfg),
        "Error creating broadcasting thread");

  int rd2st_msgs = 0;
  int st2enc_msgs = 0;
  int enc2brd_msgs = 0;
  while (1) {
    sleep(radio_config->stats_interval);
    if (broadcast_status != 0) {
      rd2st_msgs = rb_size(fread2stretch);
      st2enc_msgs = rb_size(stretch2encode);
      enc2brd_msgs = rb_size(encode2broadcast);
      logger("SlowRadio", "Messages: reader %d stretcher %d encoder %d broadcast", rd2st_msgs, st2enc_msgs, enc2brd_msgs);
    } else {
      err_logger("SlowRadio", "Stopped Broadcasting!");
      break;
    }
  }

  logger("SlowRadio", "Stopping");
 error:
  logger("SlowRadio", "Cleaning up");
  if (radio_config != NULL) destroy_config(radio_config);

  cleanup_pipe(fread2stretch, "Read to Stretch");
  cleanup_pipe(stretch2encode, "Stretch to Encode");
  cleanup_pipe(encode2broadcast, "Encode to Broadcast");

  if (filereader_cfg != NULL) filereader_config_destroy(filereader_cfg);
  if (stretcher_cfg != NULL) stretcher_config_destroy(stretcher_cfg);
  if (encoder_cfg != NULL) encoder_config_destroy(encoder_cfg);
  if (broadcast_cfg != NULL) broadcast_config_destroy(broadcast_cfg);

  // Always exit with non-zero as this is meant to be
  // a never-ending process
  return 1;
}
