#ifndef __SLOW_RADIO_MESSAGES__
#define __SLOW_RADIO_MESSAGES__

#include "filechunk.h"

#include "bclib/bstrlib.h"
#include "pstretch/audiobuffer.h"

typedef enum {
  AUDIOBUFFER,
  FILECHUNK,
  NEWTRACK,
  FINISHED
} MessageType;

typedef struct TrackInfo {
  bstring artist;
  bstring title;
} TrackInfo;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);

TrackInfo *track_info_create(bstring artist, bstring title);
void track_info_destroy(TrackInfo *info);

Message *file_chunk_message(FileChunk *chunk); /*  */
Message *audio_buffer_message(AudioBuffer *buffer);
Message *new_track_message(TrackInfo *info);
Message *finished_message();

#endif
