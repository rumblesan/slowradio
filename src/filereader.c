#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <glob.h>
#include <sndfile.h>

#include "filereader.h"
#include "virtual_ogg.h"
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

  srand(time(NULL));
  int randpos = floor(rand() / (float)RAND_MAX * list_count(filelist));
  bstring fname = bstrcpy(list_get(filelist, randpos));

  LIST_FOREACH(filelist, first, next, cur) {
    bdestroy(cur->value);
  }
  list_destroy(filelist);
  return fname;
 error:
  return bfromcstr("");
}

void *start_filereader(void *_info) {
  FileReaderInfo *info = _info;

  SF_INFO input_info;
  SNDFILE *input_file = NULL;

  float *iob = NULL;
  Message *out_message = NULL;
  int read_amount = 0;

  check(info != NULL, "FileReader: Invalid info data passed");

  int size = info->read_size;
  int channels = info->channels;
  int buffer_size = size * channels;

  log_info("FileReader: Starting");

  while (true) {
    if (input_file == NULL) {
      log_info("FileReader: Need to open new file");
      bstring newfile = get_random_file(info->pattern);
      if (blength(newfile) == 0) {
        log_err("FileReader: Could not get random file");
        continue;
      }
      log_info("FileReader: New file: %s", bdata(newfile));
      input_file = sf_open(bdata(newfile), SFM_READ, &input_info);
      if (input_file == NULL) {
        log_err("FileReader: Could not open input file. Trying another");
        continue;
      }
      bdestroy(newfile);
      if(input_info.channels != info->channels) {
        log_err("FileReader: Only accepting files with %d channels", info->channels);
      }
    }
    if (!rb_full(info->audio_out)) {
      iob = malloc(buffer_size * sizeof(float));
      check_mem(iob);

      read_amount = sf_readf_float(input_file, iob, size);
      out_message = audio_array_message(iob, channels, read_amount);
      iob = NULL;

      check(out_message != NULL,
            "FileReader: Could not create audio array message");
      rb_push(info->audio_out, out_message);
      out_message = NULL;

      if (read_amount < size) {
        sf_close(input_file);
        input_file = NULL;
      }
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
  if (input_file != NULL) sf_close(input_file);
  if (iob != NULL) free(iob);
  log_info("FileReader: Cleaned up");
  pthread_exit(NULL);
  return NULL;
}
