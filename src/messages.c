#include <stdlib.h>

#include "messages.h"
#include "filechunk.h"

#include "pstretch/audiobuffer.h"

#include "bclib/bstrlib.h"
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

Message *new_track_message(TrackInfo *info) {
  check(info != NULL, "Could not create track info");
  Message *message = message_create(NEWTRACK, info);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

Message *track_finished_message() {
  Message *message = message_create(TRACKFINISHED, NULL);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}
Message *stream_finished_message() {
  Message *message = message_create(STREAMFINISHED, NULL);
  check(message != NULL, "Could not create message");
  return message;
 error:
  return NULL;
}

TrackInfo *track_info_create(bstring artist, bstring title) {
  TrackInfo *info = malloc(sizeof(TrackInfo));
  check_mem(info);
  info->artist = artist;
  info->title  = title;
  return info;
 error:
  return NULL;
}
void track_info_destroy(TrackInfo *info) {
  check(info->artist != NULL, "Invalid artist");
  check(info->title != NULL, "Invalid artist");
  bdestroy(info->artist);
  bdestroy(info->title);
  free(info);
  return;
 error:
  return;
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
  case NEWTRACK: track_info_destroy(message->payload); break;
  case TRACKFINISHED: break;
  case STREAMFINISHED: break;
  }
  free(message);
 error:
  return;
}

const char *msg_type(Message *message) {
  check(message != NULL, "Invalid message");
  switch (message->type) {
  case AUDIOBUFFER: return "AudioBuffer";
  case FILECHUNK: return "FileChunk";
  case NEWTRACK: return "NewTrack";
  case TRACKFINISHED: return "TrackFinished";
  case STREAMFINISHED: return "StreamFinished";
  }
 error:
  return "Invalid Message";
}
