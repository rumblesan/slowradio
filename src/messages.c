#include <stdlib.h>

#include "messages.h"
#include "filechunk.h"

#include "pstretch/audiobuffer.h"

#include "bclib/dbg.h"


Message *file_chunk_message(FileChunk *chunk) {
  check(chunk != NULL, "Invalid FileChunk");
  Message *message = message_create(FILECHUNK, chunk);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

Message *audio_buffer_message(AudioBuffer *buffer) {
  check(buffer != NULL, "Invalid AudioBuffer");
  Message *message = message_create(AUDIOBUFFER, buffer);
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
  case AUDIOBUFFER: audio_buffer_destroy(message->payload); break;
  case FILECHUNK: file_chunk_destroy(message->payload); break;
  case FINISHED: log_info("Nothing to destroy for finish message"); break;
  }
  free(message);
 error:
  return;
}
