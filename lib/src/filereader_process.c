#include <stdlib.h>
#include <pthread.h>

#include <time.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "filereader_process.h"

#include "file_utils.h"
#include "ogg_utils.h"

#include "messages.h"
#include "logging.h"

#include "bclib/dbg.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

FileReaderProcessConfig *filereader_config_create(int channels,
                                                  int read_size,
                                                  bstring pattern,
                                                  int filenumber,
                                                  int thread_sleep,
                                                  int max_push_msgs,
                                                  int *status_var,
                                                  RingBuffer *pipe_out) {

  FileReaderProcessConfig *cfg = malloc(sizeof(FileReaderProcessConfig));
  check_mem(cfg);

  cfg->channels     = channels;
  cfg->read_size    = read_size;
  cfg->pattern      = pattern;
  cfg->filenumber   = filenumber;

  cfg->thread_sleep  = thread_sleep;
  cfg->max_push_msgs = max_push_msgs;

  check(status_var != NULL, "FileReader: Status_variable");
  cfg->status_var = status_var;

  check(pipe_out != NULL, "FileReader: Invalid audio out buffer passed");
  cfg->pipe_out = pipe_out;

  return cfg;
 error:
  return NULL;
}

void filereader_config_destroy(FileReaderProcessConfig *cfg) {
  bdestroy(cfg->pattern);
  free(cfg);
}

FileReaderState open_file(FileReaderProcessConfig *cfg, OggVorbis_File **vf) {

  logger("FileReader", "Need to open new file");
  TrackInfo *track_info = NULL;
  Message *out_message = NULL;
  OggVorbis_File *new_vf = NULL;
  bstring newfile = NULL;

  bool opened = false;
  while (!opened) {
    newfile = get_random_file(cfg->pattern);

    check(blength(newfile) > 0, "Could not get random file");

    logger("FileReader", "New file: %s", bdata(newfile));
    new_vf = malloc(sizeof(OggVorbis_File));
    check_mem(new_vf);

    int ovopen_err = ov_fopen(bdata(newfile), new_vf);
    if (ovopen_err) {
      err_logger("FileReader", "Could not open input file. Trying another");
      free(new_vf);
      new_vf = NULL;
      continue;
    }

    if(ov_channels(new_vf) != cfg->channels) {
      err_logger("FileReader", "Only accepting files with %d channels", cfg->channels);
      ov_clear(new_vf);
      free(new_vf);
      continue;
    }

    opened = true;
    track_info = get_track_info(new_vf);
    check(track_info != NULL, "FileReader: Could not get track info");
    out_message = new_track_message(track_info);
    check(out_message != NULL,
          "FileReader: Could not create track info message");
    rb_push(cfg->pipe_out, out_message);
    bdestroy(newfile);
  }
  *vf = new_vf;
  return READINGFILE;
 error:
  if (new_vf != NULL) free(new_vf);
  if (newfile != NULL) bdestroy(newfile);
  return FILEREADERERROR;
}

FileReaderState read_file_data(FileReaderProcessConfig *cfg, OggVorbis_File **vf) {

  AudioBuffer *out_audio = NULL;
  Message *out_message   = NULL;

  float **oggiob;

  long read_amount = ov_read_float(*vf, &oggiob, cfg->read_size, NULL);

  if (read_amount == 0) {
    rb_push(cfg->pipe_out, track_finished_message());
    ov_clear(*vf);
    free(*vf);
    *vf = NULL;
    return NOFILEOPENED;
  }

  out_audio = audio_buffer_create(cfg->channels, read_amount);
  check(out_audio != NULL, "FileReader: Could not create Audio Buffer");

  for (int c = 0; c < cfg->channels; c += 1) {
    memcpy(out_audio->buffers[c], oggiob[c], read_amount * sizeof(float));
  }
  out_message = audio_buffer_message(out_audio);
  check(out_message != NULL,
        "FileReader: Could not create audio array message");
  rb_push(cfg->pipe_out, out_message);
  out_message = NULL;
  out_audio = NULL;

  return READINGFILE;
 error:
  return FILEREADERERROR;
}


void *start_filereader(void *_cfg) {
  FileReaderProcessConfig *cfg = _cfg;

  OggVorbis_File *vf = NULL;

  *(cfg->status_var) = 1;

  srand(time(NULL));

  check(cfg != NULL, "FileReader: Invalid info data passed");

  int pushed_msgs = 0;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = cfg->thread_sleep;

  logger("FileReader", "Starting");
  FileReaderState state = NOFILEOPENED;
  int opened_files = 0;
  bool running = true;
  while (running) {

    if (state == FILEREADERERROR) {
      running = false;
    } else if (!rb_full(cfg->pipe_out) && (pushed_msgs <= cfg->max_push_msgs)) {
      pushed_msgs += 1;

      switch (state) {
      case NOFILEOPENED:
        if (cfg->filenumber == -1 || opened_files < cfg->filenumber) {
            state = open_file(cfg, &(vf));
            opened_files += 1;
        } else {
          running = false;
        }
        break;
      case READINGFILE:
        state = read_file_data(cfg, &(vf));
        break;
      case FILEREADERERROR:
        break;
        running = false;
      }

    } else {
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }

  }

 error:
  logger("FileReader", "Finished");
  *(cfg->status_var) = 0;
  if (cfg != NULL) filereader_config_destroy(cfg);
  if (vf != NULL) free(vf);
  logger("FileReader", "Cleaned up");
  return NULL;
}
