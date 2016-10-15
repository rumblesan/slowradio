#include <stdlib.h>

#include "messages.h"
#include "filechunk.h"

#include "bclib/dbg.h"

Message *file_chunk_message(FileChunk *chunk) {
  check(chunk != NULL, "Invalid FileChunk");
  Message *message = message_create(FILECHUNK, chunk);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

Message *audio_array_message(float *data, int channels, int total_length) {
  AudioArray *audio = audio_array_create(data, channels, total_length);
  check(audio != NULL, "Could not create audio array");
  Message *message = message_create(AUDIOARRAY, audio);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

Message *finished_message() {
  Message *message = message_create(FINISHED, NULL);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

Message *message_create(MessageType type, void *payload) {
  Message *message = malloc(sizeof(Message));
  check_mem(message);

  message->type    = type;
  // No check here as finished messages can have NULL payloads
  message->payload = payload;

  return message;
 error:
  return NULL;
}

void message_destroy(Message *message) {
  check(message != NULL, "Invalid message");
  switch (message->type) {
  case AUDIOARRAY: audio_array_destroy(message->payload); break;
  case FILECHUNK: file_chunk_destroy(message->payload); break;
  case FINISHED: log_info("Nothing to destroy for finish message"); break;
  }
  free(message);
 error:
  return;
}

AudioArray *audio_array_create(float *audio, int channels, int total_length) {
  AudioArray *array = malloc(sizeof(AudioArray));
  check_mem(array);

  check(audio != NULL, "Invalid audio data");
  array->audio              = audio;
  array->channels           = channels;
  array->total_length       = total_length;
  array->per_channel_length = total_length / channels;

  return array;
 error:
  return NULL;
}

void audio_array_destroy(AudioArray *arr) {
  check(arr != NULL, "Invalid audio array");
  check(arr->audio != NULL, "Invalid audio data in audio array");
  free(arr->audio);
  free(arr);
 error:
  return;
}
