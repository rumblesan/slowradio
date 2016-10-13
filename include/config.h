#ifndef __SLOW_RADIO_CONFIG__
#define __SLOW_RADIO_CONFIG__

#include "bclib/bstrlib.h"

typedef struct FileReaderConfig {
  bstring pattern;
  int read_size;
  int usleep_time;
} FileReaderConfig;

typedef struct StretcherConfig {
  double stretch;
  int window_size;
  int usleep_time;
} StretcherConfig;

typedef struct EncoderConfig {
  int samplerate;
  int usleep_time;
} EncoderConfig;

typedef struct ShoutcastConfig {
  bstring host;
  int port;
  bstring source;
  bstring password;
  bstring mount;
} ShoutcastConfig;

typedef struct RadioConfig {

  int channels;
  const char *htest;

  FileReaderConfig filereader;
  StretcherConfig stretcher;
  EncoderConfig encoder;
  ShoutcastConfig shoutcast;

} RadioConfig;

RadioConfig *read_config(char *config_path);

#endif
