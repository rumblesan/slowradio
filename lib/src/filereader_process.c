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
                                                  int thread_sleep,
                                                  int max_push_msgs,
                                                  RingBuffer *pipe_out) {

  FileReaderProcessConfig *cfg = malloc(sizeof(FileReaderProcessConfig));
  check_mem(cfg);

  cfg->channels     = channels;
  cfg->read_size    = read_size;
  cfg->pattern      = pattern;

  cfg->thread_sleep  = thread_sleep;
  cfg->max_push_msgs = max_push_msgs;

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

void *start_filereader(void *_cfg) {
  FileReaderProcessConfig *cfg = _cfg;

  OggVorbis_File *vf = NULL;
  bool opened = false;

  float **oggiob = NULL;
  AudioBuffer *out_audio = NULL;
  Message *out_message = NULL;
  long read_amount = 0;
  TrackInfo *track_info = NULL;

  srand(time(NULL));

  check(cfg != NULL, "FileReader: Invalid info data passed");

  int size = cfg->read_size;
  int channels = cfg->channels;
  int pc_size = size / channels;
  int pc_read = 0;

  int pushed_msgs = 0;

  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = cfg->thread_sleep;

  oggiob = malloc(channels * sizeof(float *));
  check_mem(oggiob);
  for (int c = 0; c < channels; c += 1) {
    oggiob[c] = malloc(pc_size * sizeof(float));
    check_mem(oggiob[c]);
  }

  logger("FileReader", "Starting");
  int current_section;

  while (true) {
    if (!opened && !rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {
      pushed_msgs += 1;
      logger("FileReader", "Need to open new file");
      bstring newfile = get_random_file(cfg->pattern);
      if (blength(newfile) == 0) {
        err_logger("FileReader", "Could not get random file");
        continue;
      }
      logger("FileReader", "New file: %s", bdata(newfile));
      vf = malloc(sizeof(OggVorbis_File));
      check_mem(vf);

      int ovopen_err = ov_fopen(bdata(newfile), vf);
      if (ovopen_err) {
        err_logger("FileReader", "Could not open input file. Trying another");
        opened = false;
        free(vf);
        vf = NULL;
        continue;
      }
      if(ov_channels(vf) != cfg->channels) {
        err_logger("FileReader", "Only accepting files with %d channels", cfg->channels);
        ov_clear(vf);
        free(vf);
        vf = NULL;
        continue;
      }
      opened = true;
      track_info = get_track_info(vf);
      out_message = new_track_message(track_info);
      check(out_message != NULL,
            "FileReader: Could not create track info message");
      rb_push(cfg->pipe_out, out_message);
      bdestroy(newfile);

    } else if (!rb_full(cfg->pipe_out) && pushed_msgs < cfg->max_push_msgs) {

      read_amount = ov_read_float(vf, &oggiob, size, &current_section);

      if (read_amount == 0) {
        rb_push(cfg->pipe_out, track_finished_message());
        ov_clear(vf);
        opened = false;
        continue;
      }

      pc_read = read_amount / channels;
      out_audio = audio_buffer_create(channels, pc_read);
      check(out_audio != NULL, "FileReader: Could not create Audio Buffer");
      for (int c = 0; c < channels; c += 1) {
        memcpy(out_audio->buffers[c], oggiob[c], pc_read * sizeof(float));
      }
      out_message = audio_buffer_message(out_audio);
      check(out_message != NULL,
            "FileReader: Could not create audio array message");
      rb_push(cfg->pipe_out, out_message);
      out_message = NULL;
      out_audio = NULL;

    } else {
      pushed_msgs = 0;
      sched_yield();
      nanosleep(&tim, &tim2);
    }
  }

 error:
  logger("FileReader", "Finished");
  if (cfg != NULL) filereader_config_destroy(cfg);
  if (oggiob != NULL) {
    if (oggiob[0] != NULL) free(oggiob[0]);
    if (oggiob[1] != NULL) free(oggiob[1]);
    free(oggiob);
  }
  if (vf != NULL) free(vf);
  logger("FileReader", "Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
