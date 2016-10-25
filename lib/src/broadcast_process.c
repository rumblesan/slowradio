#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <shout/shout.h>

#include "broadcast_process.h"

#include "messages.h"
#include "logging.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

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
                                                int *status_var,
                                                RingBuffer *pipe_in
                                                ) {

  BroadcastProcessConfig *cfg = malloc(sizeof(BroadcastProcessConfig));
  check_mem(cfg);

  check(host != NULL, "Broadcast: Invalid host passed");
  cfg->host = host;
  cfg->port = port;

  check(user != NULL, "Broadcast: Invalid user passed");
  cfg->user = user;
  check(pass != NULL, "Broadcast: Invalid pass passed");
  cfg->pass = pass;

  check(mount != NULL, "Broadcast: Invalid mount passed");
  cfg->mount = mount;


  check(name != NULL, "Broadcast: Invalid name passed");
  cfg->stream_name = name;
  check(description != NULL, "Broadcast: Invalid description passed");
  cfg->stream_description = description;
  check(genre != NULL, "Broadcast: Invalid genre passed");
  cfg->stream_genre = genre;
  check(url != NULL, "Broadcast: Invalid url passed");
  cfg->stream_url = url;

  cfg->protocol = protocol;
  cfg->format   = format;

  cfg->status_var = status_var;

  check(pipe_in != NULL, "Broadcast: Invalid pipe in");
  cfg->pipe_in = pipe_in;

  return cfg;
 error:
  return NULL;
}

void broadcast_config_destroy(BroadcastProcessConfig *cfg) {
  check(cfg != NULL, "Broadcast: Invalid cfg");
  check(cfg->host != NULL, "Broadcast: Invalid host value");
  bdestroy(cfg->host);
  check(cfg->user != NULL, "Broadcast: Invalid user value");
  bdestroy(cfg->user);
  check(cfg->pass != NULL, "Broadcast: Invalid pass value");
  bdestroy(cfg->pass);
  check(cfg->mount != NULL, "Broadcast: Invalid mount value");
  bdestroy(cfg->mount);

  check(cfg->stream_name != NULL, "Broadcast: Invalid name value");
  bdestroy(cfg->stream_name);
  check(cfg->stream_description != NULL, "Broadcast: Invalid description value");
  bdestroy(cfg->stream_description);
  check(cfg->stream_genre != NULL, "Broadcast: Invalid genre value");
  bdestroy(cfg->stream_genre);
  check(cfg->stream_url != NULL, "Broadcast: Invalid url value");
  bdestroy(cfg->stream_url);

  free(cfg);
  return;
 error:
  err_logger("Broadcast", "Could not destroy cfg");
}

void *start_broadcast(void *_cfg) {

  BroadcastProcessConfig *cfg = _cfg;

  *(cfg->status_var) = 1;

  int startup_wait = 2;
  while (1) {
    if (rb_size(cfg->pipe_in) > 50) {
      logger("Broadcast", "Input ready");
      break;
    } else {
      logger("Broadcast",
             "Waiting for input. Only %d messages in pipe",
             rb_size(cfg->pipe_in));
      sched_yield();
      sleep(startup_wait);
    }
  }

  shout_init();

  shout_t *shout = shout_new();

  check(shout != NULL,
        "Broadcast: Could not allocate shout_t: %s", shout_get_error(shout));

  check(shout_set_host(shout, bdata(cfg->host)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting hostname: %s", shout_get_error(shout));

  check(shout_set_protocol(shout, cfg->protocol) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting protocol: %s", shout_get_error(shout));

  check(shout_set_port(shout, cfg->port) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting port: %s", shout_get_error(shout));

  check(shout_set_password(shout, bdata(cfg->pass)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting password: %s", shout_get_error(shout));

  check(shout_set_mount(shout, bdata(cfg->mount)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting mount: %s", shout_get_error(shout));

  check(shout_set_user(shout, bdata(cfg->user)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting user: %s", shout_get_error(shout));

  check(shout_set_format(shout, cfg->format) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting format: %s", shout_get_error(shout));

  /* Stream MetaData */
  check(shout_set_name(shout, bdata(cfg->stream_name)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting name: %s", shout_get_error(shout));
  check(shout_set_description(shout, bdata(cfg->stream_description)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting description: %s", shout_get_error(shout));
  check(shout_set_genre(shout, bdata(cfg->stream_genre)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting genre: %s", shout_get_error(shout));
  check(shout_set_url(shout, bdata(cfg->stream_url)) == SHOUTERR_SUCCESS,
        "Broadcast: Error setting url: %s", shout_get_error(shout));

  check(shout_open(shout) == SHOUTERR_SUCCESS,
        "Broadcast: Error connecting: %s", shout_get_error(shout));

  Message *input_msg;
  FileChunk *input_audio;

  logger("Broadcast", "Connected to server...");
  while (true) {
    input_msg = rb_pop(cfg->pipe_in);
    if (input_msg == NULL) {
      err_logger("Broadcast", "Could not get input message");
      sched_yield();
      continue;
    }
    if (input_msg->type == STREAMFINISHED) {
      logger("Broadcast", "Finished message received");
      message_destroy(input_msg);
      break;
    } else if (input_msg->type == FILECHUNK) {
      input_audio = input_msg->payload;
      int ret = shout_send(shout, input_audio->data, input_audio->length);
      check(ret == SHOUTERR_SUCCESS,
            "Broadcast: Send error: %s", shout_get_error(shout));
      message_destroy(input_msg);
    } else {
      err_logger("Broadcast", "Received invalid message of type %d", input_msg->type);
      message_destroy(input_msg);
    }
    sched_yield();
    shout_sync(shout);
  }

 error:
  logger("Broadcast", "Finished");
  *(cfg->status_var) = 1;
  if (shout) {
    shout_close(shout);
    shout_shutdown();
  }
  broadcast_config_destroy(cfg);
  logger("Broadcast", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}

