#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include <shout/shout.h>

#include "shoutcast.h"

#include "filechunk.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

ShoutCastInfo *shoutcast_info_create(bstring host,
                                     int port,
                                     bstring user,
                                     bstring pass,
                                     bstring mount,
                                     RingBuffer *audio
                                     ) {

  ShoutCastInfo *info = malloc(sizeof(ShoutCastInfo));
  check_mem(info);

  check(host != NULL, "Invalid host passed");
  info->host = host;
  info->port = port;

  check(user != NULL, "Invalid user passed");
  info->user = user;
  check(pass != NULL, "Invalid pass passed");
  info->pass = pass;

  check(mount != NULL, "Invalid mount passed");
  info->mount = mount;

  info->protocol = SHOUT_PROTOCOL_HTTP;
  info->format = SHOUT_FORMAT_OGG;

  check(audio != NULL, "Invalid audio ring buffer passed");
  info->audio = audio;

  return info;
 error:
  return NULL;
}

void shoutcast_info_destroy(ShoutCastInfo *info) {
  check(info != NULL, "Invalid ShoutCastInfo value");
  check(info->host != NULL, "Invalid host value");
  bdestroy(info->host);
  check(info->user != NULL, "Invalid user value");
  bdestroy(info->user);
  check(info->pass != NULL, "Invalid pass value");
  bdestroy(info->pass);
  check(info->mount != NULL, "Invalid mount value");
  bdestroy(info->mount);
  free(info);
  return;
 error:
  log_err("Could not destroy shoutcast info");
}

void *start_shoutcast(void *_info) {

  ShoutCastInfo *info = _info;

  shout_init();

  shout_t *shout = shout_new();

  check(shout != NULL,
        "Could not allocate shout_t: %s", shout_get_error(shout));

  check(shout_set_host(shout, bdata(info->host)) == SHOUTERR_SUCCESS,
        "Error setting hostname: %s", shout_get_error(shout));

  check(shout_set_protocol(shout, info->protocol) == SHOUTERR_SUCCESS,
        "Error setting protocol: %s", shout_get_error(shout));

  check(shout_set_port(shout, info->port) == SHOUTERR_SUCCESS,
        "Error setting port: %s", shout_get_error(shout));

  check(shout_set_password(shout, bdata(info->pass)) == SHOUTERR_SUCCESS,
        "Error setting password: %s", shout_get_error(shout));

  check(shout_set_mount(shout, bdata(info->mount)) == SHOUTERR_SUCCESS,
        "Error setting mount: %s", shout_get_error(shout));

  check(shout_set_user(shout, bdata(info->user)) == SHOUTERR_SUCCESS,
        "Error setting user: %s", shout_get_error(shout));

  check(shout_set_format(shout, info->format) == SHOUTERR_SUCCESS,
        "Error setting format: %s", shout_get_error(shout));

  check(shout_open(shout) == SHOUTERR_SUCCESS,
        "Error connecting: %s", shout_get_error(shout));

  log_info("Connected to server...\n");

  while (1) {
    if (!rb_empty(info->audio)) {
      log_info("Audio available. Starting stream.");
      break;
    } else {
      log_info("Waiting for input audio...");
      sleep(2);
    }
  }

  while (1) {
    FileChunk *chunk = rb_pop(info->audio);
    check(chunk != NULL, "Could not get audio from audio in");
    int ret = shout_send(shout, chunk->data, chunk->length);
    file_chunk_destroy(chunk);
    check(ret == SHOUTERR_SUCCESS,
          "DEBUG: Send error: %s", shout_get_error(shout));
    shout_sync(shout);
  }

 error:
  log_err("Killing shoutcast thread");
  if (shout) {
    shout_close(shout);
    shout_shutdown();
  }
  shoutcast_info_destroy(info);
  pthread_exit(NULL);
  return NULL;
}

