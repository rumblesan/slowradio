#ifndef __SLOW_RADIO_CONFIG__
#define __SLOW_RADIO_CONFIG__

#include "bclib/bstrlib.h"

typedef struct FileReaderInputCfg {
  bstring pattern;
  int read_size;
  int usleep_time;
} FileReaderInputCfg;

typedef struct StretcherInputCfg {
  double stretch;
  int window_size;
  int usleep_time;
} StretcherInputCfg;

typedef struct EncoderInputCfg {
  int samplerate;
  int usleep_time;
  double quality;
} EncoderInputCfg;

typedef struct ShoutcastInputCfg {
  bstring host;
  int port;
  bstring source;
  bstring password;
  bstring mount;
  bstring name;
  bstring description;
  bstring genre;
  bstring url;
} ShoutcastInputCfg;

typedef struct RadioInputCfg {

  int channels;
  const char *htest;

  FileReaderInputCfg filereader;
  StretcherInputCfg stretcher;
  EncoderInputCfg encoder;
  ShoutcastInputCfg shoutcast;

} RadioInputCfg;

RadioInputCfg *read_config(char *config_path);

#endif
