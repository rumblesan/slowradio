#ifndef __SLOW_RADIO_MESSAGES__
#define __SLOW_RADIO_MESSAGES__

#include "filechunk.h"

typedef enum {AUDIOARRAY, FILECHUNK, FINISHED} MessageType;

typedef struct Message {
  MessageType type;
  void *payload;
} Message;

Message *message_create(MessageType type, void *payload);

void message_destroy(Message *message);


typedef struct AudioArray {
  float *audio;
  int channels;
  int total_length;
  int per_channel_length;
} AudioArray;

AudioArray *audio_array_create(float *data, int channels, int total_length);

void audio_array_destroy(AudioArray *audio);

Message *file_chunk_message(FileChunk *chunk);
Message *audio_array_message(float *data, int channels, int total_length);
Message *finished_message();

#endif
