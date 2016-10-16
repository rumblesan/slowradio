#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include "encoder_process.h"

#include "ogg_encoder.h"
#include "filechunk.h"
#include "messages.h"

#include "util.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

EncoderProcessState *encoder_process_state_create(int channels,
                                                  int samplerate,
                                                  int format,
                                                  double quality,
                                                  int usleep_time,
                                                  RingBuffer *pipe_in,
                                                  RingBuffer *pipe_out) {

  EncoderProcessState *state = malloc(sizeof(EncoderProcessState));
  check_mem(state);

  check(pipe_in != NULL, "Invalid msg in buffer passed");
  state->pipe_in = pipe_in;

  check(pipe_out != NULL, "Invalid msg out buffer passed");
  state->pipe_out = pipe_out;

  state->channels    = channels;
  state->samplerate  = samplerate;
  state->format      = format;
  state->quality     = quality;
  state->usleep_time = usleep_time;

  return state;
 error:
  return NULL;
}

void encoder_process_state_destroy(EncoderProcessState *state) {
  free(state);
}

EncoderState waiting_for_file_state(EncoderProcessState *info, OggEncoderState *encoder, Message *input_msg) {

  if (input_msg->type == NEWTRACK) {
    log_info("Encoder: New Track received");

    log_info("Encoder: Creating new encoder");
    OggEncoderState *new_encoder = ogg_encoder_state(info->channels, info->samplerate, info->quality);
    (*encoder) = *(new_encoder);

    TrackInfo *tinfo = input_msg->payload;
    check(tinfo != NULL, "Could not get track info from message");

    set_metadata(encoder, tinfo);

    FileChunk *headers = file_chunk_create();
    check(headers != NULL, "Could not create headers file chunk");
    write_headers(encoder, headers);
    Message *output_msg = file_chunk_message(headers);
    check(output_msg != NULL, "Could not create headers message");

    rb_push(info->pipe_out, output_msg);

    message_destroy(input_msg);
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
  FileChunk *audio_data = NULL;
  Message *output_msg   = NULL;

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

    bool oggfinished = false;
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

  OggEncoderState encoder;

  EncoderState state = WAITINGFORFILE;

  check(info != NULL, "Encoder: Invalid info data passed");

  log_info("Encoder: Waiting for input");
  check(wait_for_input(info->pipe_in, 1, 60), "Encoder: Could not get input in time");

  log_info("Encoder: Starting");
  bool running = true;
  while (running) {

    if (pipes_ready(info->pipe_in, info->pipe_out)) {
      Message *input_msg = rb_pop(info->pipe_in);
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
