#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "encoder_process.h"

#include "ogg_encoder.h"
#include "filechunk.h"
#include "messages.h"
#include "logging.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

EncoderProcessConfig *encoder_config_create(int channels,
                                            int samplerate,
                                            int format,
                                            double quality,
                                            int thread_sleep,
                                            int max_push_msgs,
                                            int *status_var,
                                            RingBuffer *pipe_in,
                                            RingBuffer *pipe_out) {

  EncoderProcessConfig *cfg = malloc(sizeof(EncoderProcessConfig));
  check_mem(cfg);

  check(pipe_in != NULL, "Invalid msg in buffer passed");
  cfg->pipe_in = pipe_in;

  check(pipe_out != NULL, "Invalid msg out buffer passed");
  cfg->pipe_out = pipe_out;

  cfg->channels     = channels;
  cfg->samplerate   = samplerate;
  cfg->format       = format;
  cfg->quality      = quality;

  cfg->status_var = status_var;

  cfg->thread_sleep  = thread_sleep;
  cfg->max_push_msgs = max_push_msgs;

  return cfg;
 error:
  return NULL;
}

void encoder_config_destroy(EncoderProcessConfig *cfg) {
  free(cfg);
}

EncoderState waiting_for_file_state(EncoderProcessConfig *cfg, OggEncoderState **encoderP, Message *input_msg) {

  if (input_msg->type == NEWTRACK) {
    logger("Encoder", "New Track received. Creating new encoder");

    OggEncoderState *new_encoder = ogg_encoder_state(cfg->channels, cfg->samplerate, cfg->quality);
    check(new_encoder != NULL, "Could not create new encoder state");
    *encoderP = new_encoder;

    TrackInfo *tinfo = input_msg->payload;
    check(tinfo != NULL, "Could not get track info from message");

    set_metadata(new_encoder, tinfo);

    FileChunk *headers = file_chunk_create();
    check(headers != NULL, "Could not create headers file chunk");
    write_headers(new_encoder, headers);
    Message *output_msg = file_chunk_message(headers);
    check(output_msg != NULL, "Could not create headers message");

    rb_push(cfg->pipe_out, output_msg);

    message_destroy(input_msg);
    logger("Encoder", "Changing to encoding state");
    return ENCODINGFILE;
  } else if (input_msg->type == STREAMFINISHED) {
    logger("Encoder", "Stream Finished message received");
    message_destroy(input_msg);
    return CLOSINGSTREAM;
  } else {
    err_logger("Encoder", "Received message of type %s but waiting for new track", msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }
 error:
  return ENCODERERROR;
}

EncoderState encoding_file_state(EncoderProcessConfig *cfg, OggEncoderState **encoderP, Message *input_msg) {
  FileChunk *audio_data = NULL;
  Message *output_msg   = NULL;

  check(encoderP != NULL, "Invalid encoder passed");
  OggEncoderState *encoder = *encoderP;
  check(encoder != NULL, "Invalid encoder passed");

  if (input_msg->type == AUDIOBUFFER) {
    AudioBuffer *audio = input_msg->payload;
    check(audio != NULL, "Received invalid audio");
    add_audio(encoder, audio);

    audio_data = file_chunk_create();
    check(audio_data != NULL, "Could not create audio data");
    write_audio(encoder, audio_data);

    if (audio_data->data == NULL) {
      free(audio_data);
    } else {
      output_msg = file_chunk_message(audio_data);
      check(output_msg != NULL, "Could not create audio message");
      rb_push(cfg->pipe_out, output_msg);
    }

    message_destroy(input_msg);
    return ENCODINGFILE;

  } else if (input_msg->type == STREAMFINISHED) {
    logger("Encoder", "Stream Finished message received");
    message_destroy(input_msg);
    return CLOSINGSTREAM;
  } else if (input_msg->type == TRACKFINISHED) {
    logger("Encoder", "Track Finished message received");

    // We need to make sure the ogg stream is emptied
    file_finished(encoder);
    audio_data = file_chunk_create();
    check(audio_data != NULL, "Could not create audio data");

    bool oggfinished = false;
    while (!oggfinished) {
      oggfinished = write_audio(encoder, audio_data);
    }

    if (audio_data->data == NULL) {
      free(audio_data);
    } else {
      output_msg = file_chunk_message(audio_data);
      check(output_msg != NULL, "Could not create audio message");
      rb_push(cfg->pipe_out, output_msg);
    }

    cleanup_encoder(encoder);
    *encoderP = NULL;
    message_destroy(input_msg);
    logger("Encoder", "Changing to waiting for file state");
    return WAITINGFORFILE;
  } else {
    err_logger("Encoder", "Received invalid %s message whilst in encoding state",
            msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }

 error:
  return ENCODERERROR;
}

void *start_encoder(void *_cfg) {
  EncoderProcessConfig *cfg = _cfg;

  OggEncoderState *encoder = NULL;

  EncoderState state = WAITINGFORFILE;
  int pushed_msgs = 0;

  check(cfg != NULL, "Encoder: Invalid config data passed");

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = cfg->thread_sleep;

  logger("Encoder", "Starting");
  *(cfg->status_var) = 1;
  bool running = true;
  while (running) {

    if (state == CLOSINGSTREAM || state == ENCODERERROR) {
      running = false;
    }

    if (!rb_empty(cfg->pipe_in) && !rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;

      Message *input_msg = rb_pop(cfg->pipe_in);
      check(input_msg != NULL, "Encoder: Could not get input message");

      switch(state) {
      case WAITINGFORFILE:
        state = waiting_for_file_state(cfg, &encoder, input_msg);
        break;
      case ENCODINGFILE:
        state = encoding_file_state(cfg, &encoder, input_msg);
        break;
      case CLOSINGSTREAM:
        running = false;
        break;
      case ENCODERERROR:
        running = false;
        break;
      }

    } else {
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }

  }

 error:
  logger("Encoder", "Finished");
  *(cfg->status_var) = 0;
  if (cfg != NULL) encoder_config_destroy(cfg);
  if (encoder != NULL) cleanup_encoder(encoder);
  logger("Encoder", "Cleaned up");
  return NULL;
}
