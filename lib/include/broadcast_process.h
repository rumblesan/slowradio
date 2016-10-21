#ifndef __SLOW_RADIO_BROADCAST_PROCESS__
#define __SLOW_RADIO_BROADCAST_PROCESS__

#include <shout/shout.h>

#include "bclib/ringbuffer.h"
#include "bclib/bstrlib.h"

typedef struct BroadcastProcessConfig {

  bstring host;
  int port;

  bstring user;
  bstring pass;

  bstring stream_name;
  bstring stream_description;
  bstring stream_genre;
  bstring stream_url;

  bstring mount;
  int protocol;
  int format;

  RingBuffer *audio;

} BroadcastProcessConfig;

BroadcastProcessConfig *broadcast_config_create(bstring host,
                                                int port,
                                                bstring user,
                                                bstring pass,
                                                bstring mount,
                                                bstring name,
                                                bstring description,
                                                bstring genre,
                                                bstring url,
                                                int protocol,
                                                int format,
                                                RingBuffer *audio);

void broadcast_config_destroy(BroadcastProcessConfig *cfg);

void *start_broadcast(void *_cfg);

#endif
