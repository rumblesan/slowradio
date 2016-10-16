#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "encoder_process.h"

#include "ogg_encoder.h"
#include "filechunk.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

EncoderProcessState *encoder_process_state_create(int channels,
                                                  int samplerate,
                                                  int format,
                                                  int usleep_time,
                                                  RingBuffer *msg_in,
                                                  RingBuffer *msg_out) {

  EncoderProcessState *state = malloc(sizeof(EncoderProcessState));
  check_mem(state);

  check(msg_in != NULL, "Invalid msg in buffer passed");
  state->pipe_in = msg_in;

  check(msg_out != NULL, "Invalid msg out buffer passed");
  state->pipe_out = msg_out;

  state->channels    = channels;
  state->samplerate  = samplerate;
  state->format      = format;
  state->usleep_time = usleep_time;

  return state;
 error:
  return NULL;
}

void encoder_process_state_destroy(EncoderProcessState *state) {
  free(state);
}

EncoderState waiting_for_file_state(EncoderProcessState *info, OggEncoderState *encoder, Message *input_msg) {
  TrackInfo *tinfo    = NULL;
  FileChunk *headers  = NULL;
  Message *output_msg = NULL;

  // TODO move this into info
  float quality       = 0.5;

  if (input_msg->type == NEWTRACK) {
    log_info("Encoder: New Track received");
    tinfo = input_msg->payload;
    log_info("New Track: %s - %s", bdata(tinfo->artist), bdata(tinfo->title));
    message_destroy(input_msg);

    (*encoder) = *(ogg_encoder_state(info->channels, info->samplerate, quality));
    set_headers(encoder);
    headers = file_chunk_create();
    check(headers != NULL, "Could not create headers file chunk");

    write_headers(encoder, headers);
    output_msg = file_chunk_message(headers);
    check(output_msg != NULL, "Could not create headers message");

    rb_push(info->pipe_out, output_msg);
    return ENCODINGFILE;
  } else {
    log_err("Encoder: Received message of type %s but waiting for new track", msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }
 error:
  return ENCODERERROR;
}

EncoderState encoding_file_state(EncoderProcessState *info, OggEncoderState *encoder, Message *input_msg) {
  AudioBuffer *audio    = NULL;
  FileChunk *audio_data = NULL;
  Message *output_msg   = NULL;

  if (input_msg->type == AUDIOBUFFER) {
    audio = input_msg->payload;
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
      rb_push(info->pipe_out, output_msg);
    }
    message_destroy(input_msg);
    return ENCODINGFILE;
  } else if (input_msg->type == STREAMFINISHED) {
    log_info("Encoder: Stream Finished message received");
    message_destroy(input_msg);
    return CLOSINGSTREAM;
  } else if (input_msg->type == TRACKFINISHED) {
    log_info("Encoder: Track Finished message received");

    // We need to make sure the ogg stream is emptied
    file_finished(encoder);
    audio_data = file_chunk_create();
    check(audio_data != NULL, "Could not create audio data");
    int oggfinished = 0;
    while (!oggfinished) {
      oggfinished = write_audio(encoder, audio_data);
    }

    if (audio_data->data == NULL) {
      free(audio_data);
    } else {
      output_msg = file_chunk_message(audio_data);
      check(output_msg != NULL, "Could not create audio message");
      rb_push(info->pipe_out, output_msg);
    }

    cleanup_encoder(encoder);
    message_destroy(input_msg);
    return WAITINGFORFILE;
  } else {
    log_err("Encoder: Received invalid %s message whilst in encoding state",
            msg_type(input_msg));
    message_destroy(input_msg);
    return ENCODERERROR;
  }

 error:
  return ENCODERERROR;
}

void *start_encoder_process(void *_info) {
  EncoderProcessState *info = _info;

  Message *input_msg = NULL;

  OggEncoderState encoder;

  EncoderState state = WAITINGFORFILE;

  bool running = true;

  check(info != NULL, "Encoder: Invalid info data passed");

  int startup_wait = 1;
  while (true) {
    if (!rb_empty(info->pipe_in)) {
      log_info("Encoder: Audio available");
      break;
    } else {
      log_info("Encoder: Waiting for input audio...");
      sleep(startup_wait);
    }
  }
  log_info("Encoder: Starting");

  while (running) {

    if (!rb_full(info->pipe_out) && !rb_empty(info->pipe_in)) {
      input_msg = rb_pop(info->pipe_in);
      check(input_msg != NULL, "Encoder: Could not get input message");

      switch(state) {
      case WAITINGFORFILE:
        state = waiting_for_file_state(info, &encoder, input_msg);
        break;
      case ENCODINGFILE:
        state = encoding_file_state(info, &encoder, input_msg);
        break;
      case CLOSINGSTREAM:
        running = false;
        break;
      case ENCODERERROR:
        running = false;
        break;
      }

    } else {
      sched_yield();
      usleep(info->usleep_time);
    }

  }

  while (true) {
    if (!rb_full(info->pipe_out)) {
      rb_push(info->pipe_out, stream_finished_message());
      break;
    } else {
      sched_yield();
      usleep(info->usleep_time);
    }
  }

 error:
  log_info("Encoder: Finished");
  if (info != NULL) encoder_process_state_destroy(info);
  cleanup_encoder(&encoder);
  log_info("Encoder: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
