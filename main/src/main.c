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

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"


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
  pthread_t stretcher_thread;
  pthread_t encoder_thread;
  pthread_t broadcast_thread;
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


  check(!pthread_create(&reader_thread,
                        NULL,
                        &start_filereader,
                        filereader_cfg),
        "Error creating file reader thread");

  check(!pthread_create(&stretcher_thread,
                        NULL,
                        &start_stretcher,
                        stretcher_cfg),
        "Error creating stretcher thread");

  check(!pthread_create(&encoder_thread,
                        NULL,
                        &start_encoder,
                        encoder_cfg),
        "Error creating encoder thread");

  check(!pthread_create(&broadcast_thread,
                        NULL,
                        &start_broadcast,
                        broadcast_cfg),
        "Error creating broadcasting thread");

  int rd2st_msgs = 0;
  int st2enc_msgs = 0;
  int enc2brd_msgs = 0;
  while (1) {
    sleep(30);
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

  return 0;
 error:
  logger("SlowRadio", "Cleaning up");
  // TODO create radio_config_destroy
  // if (radio_config != NULL) radio_config_destroy(radio_config);
  if (fread2stretch != NULL) rb_destroy(fread2stretch);
  if (stretch2encode != NULL) rb_destroy(stretch2encode);
  if (encode2broadcast != NULL) rb_destroy(encode2broadcast);
  if (filereader_cfg != NULL) filereader_config_destroy(filereader_cfg);
  if (stretcher_cfg != NULL) stretcher_config_destroy(stretcher_cfg);
  if (encoder_cfg != NULL) encoder_config_destroy(encoder_cfg);
  if (broadcast_cfg != NULL) broadcast_config_destroy(broadcast_cfg);

  return 1;
}
