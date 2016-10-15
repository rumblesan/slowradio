#ifndef __SLOW_RADIO_MESSAGES__
#define __SLOW_RADIO_MESSAGES__

#include "filechunk.h"

#include "pstretch/audiobuffer.h"

typedef enum {AUDIOBUFFER, FILECHUNK, FINISHED} MessageType;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);

Message *file_chunk_message(FileChunk *chunk); /*  */
Message *audio_buffer_message(AudioBuffer *buffer);
Message *finished_message();

#endif
