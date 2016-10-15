#include <stdlib.h>

#include <libconfig.h>

#include "config.h"

#include "bclib/dbg.h"

RadioConfig *read_config(char *config_path) {
  log_info("config path %s", config_path);
  config_t config;
  config_t *cfg = &config;

  RadioConfig *radio_config = malloc(sizeof(RadioConfig));
  check_mem(radio_config);

  config_init(cfg);

  int readok;

  readok = config_read_file(cfg, config_path);
  check(readok, "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));

  // Read channels
  readok = config_lookup_int(cfg, "channels", &(radio_config->channels));
  check(readok, "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));


  // File Reader config
  config_setting_t *frsetting = config_lookup(cfg, "filereader");
  check(frsetting != NULL, "Could not load filereader config");
  int frreadsize = config_setting_lookup_int(frsetting, "read_size",
                                             &(radio_config->filereader.read_size));
  const char *file_pattern;
  int frpattern = config_setting_lookup_string(frsetting, "pattern", &(file_pattern));
  int frsleep = config_setting_lookup_int(frsetting, "usleep_time",
                                          &(radio_config->filereader.usleep_time));
  check(frreadsize && frpattern && frsleep,
        "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));
  radio_config->filereader.pattern = bfromcstr(file_pattern);

  // Stretcher config
  config_setting_t *strsetting = config_lookup(cfg, "stretcher");
  check(strsetting != NULL, "Could not load stretcher config");
  int strstretch = config_setting_lookup_float(strsetting, "stretch",
                                               &(radio_config->stretcher.stretch));
  int strwindow = config_setting_lookup_int(strsetting, "window_size",
                                            &(radio_config->stretcher.window_size));
  int strsleep = config_setting_lookup_int(strsetting, "usleep_time",
                                           &(radio_config->stretcher.usleep_time));
  check(strstretch && strwindow && strsleep,
        "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));

  config_setting_t *encsetting = config_lookup(cfg, "encoder");
  check(encsetting != NULL, "Could not load encoder config");
  int encsamplerate = config_setting_lookup_int(encsetting, "samplerate",
                                             &(radio_config->encoder.samplerate));
  int encsleep = config_setting_lookup_int(encsetting, "usleep_time",
                                           &(radio_config->encoder.usleep_time));
  check(encsamplerate && encsleep,
        "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));

  config_setting_t *shoutsetting = config_lookup(cfg, "shoutcast");
  check(shoutsetting != NULL, "Could not load shoutcast config");

  const char *shout_host;
  const char *shout_source;
  const char *shout_pass;
  const char *shout_mount;
  const char *shout_name;
  const char *shout_description;
  const char *shout_genre;
  const char *shout_url;
  check(config_setting_lookup_string(shoutsetting, "host", &(shout_host)) &&
        config_setting_lookup_int(shoutsetting, "port", &(radio_config->shoutcast.port)) &&
        config_setting_lookup_string(shoutsetting, "source", &(shout_source)) &&
        config_setting_lookup_string(shoutsetting, "password", &(shout_pass)) &&
        config_setting_lookup_string(shoutsetting, "mount", &(shout_mount)) && 
        config_setting_lookup_string(shoutsetting, "name", &(shout_name)) &&
        config_setting_lookup_string(shoutsetting, "description", &(shout_description)) &&
        config_setting_lookup_string(shoutsetting, "genre", &(shout_genre)) &&
        config_setting_lookup_string(shoutsetting, "url", &(shout_url)),

        "%s:%d - %s", config_error_file(cfg),
        config_error_line(cfg), config_error_text(cfg));

  radio_config->shoutcast.host        = bfromcstr(shout_host);
  radio_config->shoutcast.source      = bfromcstr(shout_source);
  radio_config->shoutcast.password    = bfromcstr(shout_pass);
  radio_config->shoutcast.mount       = bfromcstr(shout_mount);
  radio_config->shoutcast.name        = bfromcstr(shout_name);
  radio_config->shoutcast.description = bfromcstr(shout_description);
  radio_config->shoutcast.genre       = bfromcstr(shout_genre);
  radio_config->shoutcast.url         = bfromcstr(shout_url);

  if (cfg != NULL) config_destroy(cfg);
  return radio_config;
 error:
  if (cfg != NULL) config_destroy(cfg);
  return NULL;
};


