#ifndef __SLOW_RADIO_MESSAGES__
#define __SLOW_RADIO_MESSAGES__

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


typedef struct FileChunk {
  unsigned char *data;
  int length;
} FileChunk;

FileChunk *file_chunk_create(unsigned char *data, int length);

void file_chunk_destroy(FileChunk *chunk);

Message *file_chunk_message(unsigned char *data, int length);
Message *audio_array_message(float *data, int channels, int total_length);
Message *finished_message();

#endif
