#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <glob.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "filereader.h"
#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/list.h"
#include "bclib/bstrlib.h"
#include "bclib/ringbuffer.h"

FileReaderInfo *filereader_info_create(int channels,
                                       int read_size,
                                       bstring pattern,
                                       int usleep_amount,
                                       RingBuffer *audio_out) {

  FileReaderInfo *info = malloc(sizeof(FileReaderInfo));
  check_mem(info);

  info->channels      = channels;
  info->read_size     = read_size;
  info->usleep_amount = usleep_amount;
  info->pattern       = pattern;

  check(audio_out != NULL, "FileReader: Invalid audio out buffer passed");
  info->audio_out = audio_out;

  return info;
 error:
  return NULL;
}

void filereader_info_destroy(FileReaderInfo *info) {
  bdestroy(info->pattern);
  free(info);
}

bool is_regular_file(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
}

bstring get_random_file(bstring pattern) {
  glob_t globbuf;

  List *filelist = list_create();
  check(filelist != NULL, "Could not create file list");

  check(!glob(bdata(pattern), GLOB_TILDE, NULL, &globbuf), "Could not glob folder");

  for(size_t i = 0; i < globbuf.gl_pathc; i += 1) {
    char *name = globbuf.gl_pathv[i];
    if (is_regular_file(name)) {
      list_push(filelist, bfromcstr(name));
    }
  }
  if(globbuf.gl_pathc > 0) globfree(&globbuf);

  int randpos = floor((rand() / (float)RAND_MAX) * list_count(filelist));
  bstring fname = bstrcpy(list_get(filelist, randpos));

  LIST_FOREACH(filelist, first, next, cur) {
    bdestroy(cur->value);
  }
  list_destroy(filelist);
  return fname;
 error:
  return bfromcstr("");
}

TrackInfo *get_track_info(OggVorbis_File *vf) {
  char **ptr = ov_comment(vf, -1)->user_comments;
  bstring artist;
  bstring title;
  while(*ptr) {
    if (strncmp(*ptr, "ARTIST", strlen("ARTIST")) == 0) {
      artist = bfromcstr(*ptr);
    } else if (strncmp(*ptr, "TITLE", strlen("TITLE")) == 0) {
      title = bfromcstr(*ptr);
    }
    ++ptr;
  }
  TrackInfo *track_info = track_info_create(artist, title);
  return track_info;
}

int ov_channels(OggVorbis_File *vf) {
  vorbis_info *vi=ov_info(vf,-1);
  return vi->channels;
}

void *start_filereader(void *_info) {
  FileReaderInfo *info = _info;

  OggVorbis_File *vf = NULL;
  bool opened = false;

  float **oggiob = NULL;
  AudioBuffer *out_audio = NULL;
  Message *out_message = NULL;
  long read_amount = 0;
  TrackInfo *track_info = NULL;

  srand(time(NULL));

  check(info != NULL, "FileReader: Invalid info data passed");

  int size = info->read_size;
  int channels = info->channels;
  int pc_size = size / channels;
  int pc_read = 0;

  oggiob = malloc(channels * sizeof(float *));
  check_mem(oggiob);
  for (int c = 0; c < channels; c += 1) {
    oggiob[c] = malloc(pc_size * sizeof(float));
    check_mem(oggiob[c]);
  }

  log_info("FileReader: Starting");
  int current_section;

  while (true) {
    if (!opened && !rb_full(info->audio_out)) {
      log_info("FileReader: Need to open new file");
      bstring newfile = get_random_file(info->pattern);
      if (blength(newfile) == 0) {
        log_err("FileReader: Could not get random file");
        continue;
      }
      log_info("FileReader: New file: %s", bdata(newfile));
      vf = malloc(sizeof(OggVorbis_File));
      check_mem(vf);

      int ovopen_err = ov_fopen(bdata(newfile), vf);
      if (ovopen_err) {
        log_err("FileReader: Could not open input file. Trying another");
        opened = false;
        free(vf);
        vf = NULL;
        continue;
      }
      if(ov_channels(vf) != info->channels) {
        log_err("FileReader: Only accepting files with %d channels", info->channels);
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
      rb_push(info->audio_out, out_message);
      bdestroy(newfile);

    } else if (!rb_full(info->audio_out)) {

      read_amount = ov_read_float(vf, &oggiob, size, &current_section);

      if (read_amount == 0) {
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
      rb_push(info->audio_out, out_message);
      out_message = NULL;
      out_audio = NULL;

    } else {
      sched_yield();
      usleep(info->usleep_amount);
    }
  }

  while (true) {
    if (!rb_full(info->audio_out)) {
      rb_push(info->audio_out, finished_message());
      break;
    } else {
      sched_yield();
      usleep(info->usleep_amount);
    }
  }

 error:
  log_info("FileReader: Finished");
  if (info != NULL) filereader_info_destroy(info);
  if (oggiob != NULL) {
    if (oggiob[0] != NULL) free(oggiob[0]);
    if (oggiob[1] != NULL) free(oggiob[1]);
    free(oggiob);
  }
  if (vf != NULL) free(vf);
  log_info("FileReader: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
