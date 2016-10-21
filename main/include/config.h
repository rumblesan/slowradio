#ifndef __SLOW_RADIO_CONFIG__
#define __SLOW_RADIO_CONFIG__

#include "bclib/bstrlib.h"

typedef struct FileReaderInputCfg {
  bstring pattern;
  int read_size;
  int thread_sleep;
} FileReaderInputCfg;

typedef struct StretcherInputCfg {
  double stretch;
  int window_size;
  int thread_sleep;
} StretcherInputCfg;

typedef struct EncoderInputCfg {
  int samplerate;
  int thread_sleep;
  double quality;
} EncoderInputCfg;

typedef struct BroadcastInputCfg {
  bstring host;
  int port;
  bstring source;
  bstring password;
  bstring mount;
  bstring name;
  bstring description;
  bstring genre;
  bstring url;
} BroadcastInputCfg;

typedef struct RadioInputCfg {

  int channels;
  const char *htest;

  FileReaderInputCfg filereader;
  StretcherInputCfg stretcher;
  EncoderInputCfg encoder;
  BroadcastInputCfg broadcast;

} RadioInputCfg;

RadioInputCfg *read_config(char *config_path);

#endif
