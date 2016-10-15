#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <shout/shout.h>

#include "shoutcast.h"

#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

ShoutCastInfo *shoutcast_info_create(bstring host,
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
                                     RingBuffer *audio
                                     ) {

  ShoutCastInfo *info = malloc(sizeof(ShoutCastInfo));
  check_mem(info);

  check(host != NULL, "Shoutcast: Invalid host passed");
  info->host = host;
  info->port = port;

  check(user != NULL, "Shoutcast: Invalid user passed");
  info->user = user;
  check(pass != NULL, "Shoutcast: Invalid pass passed");
  info->pass = pass;

  check(mount != NULL, "Shoutcast: Invalid mount passed");
  info->mount = mount;


  check(name != NULL, "Shoutcast: Invalid name passed");
  info->stream_name = name;
  check(description != NULL, "Shoutcast: Invalid description passed");
  info->stream_description = description;
  check(genre != NULL, "Shoutcast: Invalid genre passed");
  info->stream_genre = genre;
  check(url != NULL, "Shoutcast: Invalid url passed");
  info->stream_url = url;

  info->protocol = protocol;
  info->format   = format;

  check(audio != NULL, "Shoutcast: Invalid audio ring buffer passed");
  info->audio = audio;

  return info;
 error:
  return NULL;
}

void shoutcast_info_destroy(ShoutCastInfo *info) {
  check(info != NULL, "Shoutcast: Invalid info");
  check(info->host != NULL, "Shoutcast: Invalid host value");
  bdestroy(info->host);
  check(info->user != NULL, "Shoutcast: Invalid user value");
  bdestroy(info->user);
  check(info->pass != NULL, "Shoutcast: Invalid pass value");
  bdestroy(info->pass);
  check(info->mount != NULL, "Shoutcast: Invalid mount value");
  bdestroy(info->mount);

  check(info->stream_name != NULL, "Shoutcast: Invalid name value");
  bdestroy(info->stream_name);
  check(info->stream_description != NULL, "Shoutcast: Invalid description value");
  bdestroy(info->stream_description);
  check(info->stream_genre != NULL, "Shoutcast: Invalid genre value");
  bdestroy(info->stream_genre);
  check(info->stream_url != NULL, "Shoutcast: Invalid url value");
  bdestroy(info->stream_url);

  free(info);
  return;
 error:
  log_err("Shoutcast: Could not destroy info");
}

void *start_shoutcast(void *_info) {

  ShoutCastInfo *info = _info;

  int startup_wait = 2;
  while (1) {
    if (!rb_empty(info->audio)) {
      log_info("Shoutcast: Audio available.");
      break;
    } else {
      log_info("Shoutcast: Waiting for input audio...");
      sleep(startup_wait);
    }
  }

  shout_init();

  shout_t *shout = shout_new();

  check(shout != NULL,
        "Shoutcast: Could not allocate shout_t: %s", shout_get_error(shout));

  check(shout_set_host(shout, bdata(info->host)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting hostname: %s", shout_get_error(shout));

  check(shout_set_protocol(shout, info->protocol) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting protocol: %s", shout_get_error(shout));

  check(shout_set_port(shout, info->port) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting port: %s", shout_get_error(shout));

  check(shout_set_password(shout, bdata(info->pass)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting password: %s", shout_get_error(shout));

  check(shout_set_mount(shout, bdata(info->mount)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting mount: %s", shout_get_error(shout));

  check(shout_set_user(shout, bdata(info->user)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting user: %s", shout_get_error(shout));

  check(shout_set_format(shout, info->format) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting format: %s", shout_get_error(shout));

  /* Stream MetaData */
  check(shout_set_name(shout, bdata(info->stream_name)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting name: %s", shout_get_error(shout));
  check(shout_set_description(shout, bdata(info->stream_description)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting description: %s", shout_get_error(shout));
  check(shout_set_genre(shout, bdata(info->stream_genre)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting genre: %s", shout_get_error(shout));
  check(shout_set_url(shout, bdata(info->stream_url)) == SHOUTERR_SUCCESS,
        "Shoutcast: Error setting url: %s", shout_get_error(shout));

  check(shout_open(shout) == SHOUTERR_SUCCESS,
        "Shoutcast: Error connecting: %s", shout_get_error(shout));

  Message *input_msg;
  FileChunk *input_audio;

  log_info("Shoutcast: Connected to server...");
  while (true) {
    input_msg = rb_pop(info->audio);
    check(input_msg != NULL, "Shoutcast: Could not get input message");
    if (input_msg->type == FINISHED) {
      log_info("Shoutcast: Finished message received");
      message_destroy(input_msg);
      break;
    } else if (input_msg->type == FILECHUNK) {
      input_audio = input_msg->payload;
      int ret = shout_send(shout, input_audio->data, input_audio->length);
      check(ret == SHOUTERR_SUCCESS,
            "Shoutcast: Send error: %s", shout_get_error(shout));
      message_destroy(input_msg);
    } else {
      log_err("Shoutcast: Received invalid message of type %d", input_msg->type);
      message_destroy(input_msg);
    }
    shout_sync(shout);
  }

 error:
  log_info("Shoutcast: Finished");
  if (shout) {
    shout_close(shout);
    shout_shutdown();
  }
  shoutcast_info_destroy(info);
  log_info("Shoutcast: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}

