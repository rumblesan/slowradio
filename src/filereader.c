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

void debug_ov(OggVorbis_File *vf) {
  char **ptr=ov_comment(vf,-1)->user_comments;
  vorbis_info *vi=ov_info(vf,-1);
  while(*ptr){
    fprintf(stderr,"%s\n",*ptr);
    ++ptr;
  }
  fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
  fprintf(stderr,"Encoded by: %s\n\n",ov_comment(vf,-1)->vendor);
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
  float *out_audio = NULL;
  Message *out_message = NULL;
  long read_amount = 0;

  srand(time(NULL));

  check(info != NULL, "FileReader: Invalid info data passed");

  int size = info->read_size;
  int channels = info->channels;
  int buffer_size = size * channels;

  oggiob = malloc(2 * sizeof(float *));
  check_mem(oggiob);
  oggiob[0] = malloc(buffer_size * sizeof(float));
  check_mem(oggiob[0]);
  oggiob[1] = malloc(buffer_size * sizeof(float));
  check_mem(oggiob[1]);

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
      log_info("done");
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
      log_info("Debugging file");
      debug_ov(vf);
      bdestroy(newfile);

    } else if (!rb_full(info->audio_out)) {

      read_amount = ov_read_float(vf, &oggiob, size, &current_section);
      if (read_amount == 0) {
        ov_clear(vf);
        opened = false;
        continue;
      }

      out_audio = malloc(channels * read_amount * sizeof(float));
      check_mem(out_audio);

      for (int c = 0; c < channels; c += 1) {
        for (int s = 0; s < read_amount; s += 1) {
          int pos = (s * channels) + c;
          out_audio[pos] = oggiob[c][s];
        }
      }

      out_message = audio_array_message(out_audio, channels, read_amount);
      out_audio = NULL;

      check(out_message != NULL,
            "FileReader: Could not create audio array message");
      rb_push(info->audio_out, out_message);
      out_message = NULL;

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
