#ifndef __SLOW_RADIO_SHOUTCAST__
#define __SLOW_RADIO_SHOUTCAST__

#include <shout/shout.h>

#include "bclib/ringbuffer.h"
#include "bclib/bstrlib.h"

typedef struct ShoutCastInfo {

  bstring host;
  int port;

  bstring user;
  bstring pass;

  bstring mount;
  int protocol;
  int format;

  RingBuffer *audio;

} ShoutCastInfo;

typedef struct ShoutAudio {
  unsigned char *data;
  int len;
} ShoutAudio;

ShoutCastInfo *shoutcast_info_create(bstring host,
                                     int port,
                                     bstring user,
                                     bstring pass,
                                     bstring mount,
                                     RingBuffer *audio);

void shoutcast_info_destroy(ShoutCastInfo *info);

ShoutAudio *shout_audio_create(unsigned char *data, int len);
void shout_audio_destory(ShoutAudio *audio);

void *start_shoutcast(void *_info);

#endif
