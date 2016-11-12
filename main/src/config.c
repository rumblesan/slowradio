#include <stdlib.h>

#include <libconfig.h>

#include "config.h"

#include "logging.h"
#include "bclib/dbg.h"

int config_setting_lookup_bstring(const config_setting_t *setting,
                                  const char *path,
                                  bstring *value) {
  const char *stringval;
  int rv = config_setting_lookup_string(setting, path, &stringval);
  if (rv) *value = bfromcstr(stringval);
  return rv;
}

RadioInputCfg *read_config(char *config_path) {
  logger("Config", "config path %s", config_path);
  config_t config;
  config_t *cfg = &config;

  RadioInputCfg *radio_config = malloc(sizeof(RadioInputCfg));
  check_mem(radio_config);

  config_init(cfg);

  check(config_read_file(cfg, config_path),
        "Could not read config file - %s:%d - %s",
        config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));

  // Channels config
  check(config_lookup_int(cfg, "channels", &(radio_config->channels)),
        "Could not read channels setting");

  // Stats interval config
  check(config_lookup_int(cfg, "stats_interval", &(radio_config->stats_interval)),
        "Could not read stats interval setting");

  // File Reader config
  config_setting_t *frsetting = config_lookup(cfg, "filereader");
  check(frsetting != NULL &&
        config_setting_lookup_int(frsetting, "read_size", &(radio_config->filereader.read_size)) &&
        config_setting_lookup_bstring(frsetting, "pattern", &(radio_config->filereader.pattern)) &&
        config_setting_lookup_int(frsetting, "thread_sleep", &(radio_config->filereader.thread_sleep)),
        "Could not read filereader settings");

  // Stretcher config
  config_setting_t *strsetting = config_lookup(cfg, "stretcher");
  check(strsetting != NULL &&
        config_setting_lookup_float(strsetting, "stretch", &(radio_config->stretcher.stretch)) &&
        config_setting_lookup_int(strsetting, "window_size", &(radio_config->stretcher.window_size)) &&
        config_setting_lookup_int(strsetting, "thread_sleep", &(radio_config->stretcher.thread_sleep)),
        "Could not read stretcher settings");

  // Encoder config
  config_setting_t *encsetting = config_lookup(cfg, "encoder");
  check(encsetting != NULL &&
        config_setting_lookup_int(encsetting, "samplerate", &(radio_config->encoder.samplerate)) &&
        config_setting_lookup_int(encsetting, "thread_sleep", &(radio_config->encoder.thread_sleep)) &&
        config_setting_lookup_float(encsetting, "quality", &(radio_config->encoder.quality)),
        "Could not read encoder settings");

  // Broadcast config
  config_setting_t *broadcastsetting = config_lookup(cfg, "broadcast");
  check(broadcastsetting != NULL &&
        config_setting_lookup_bstring(broadcastsetting, "host", &(radio_config->broadcast.host)) &&
        config_setting_lookup_int(broadcastsetting, "port", &(radio_config->broadcast.port)) &&
        config_setting_lookup_bstring(broadcastsetting, "source", &(radio_config->broadcast.source)) &&
        config_setting_lookup_bstring(broadcastsetting, "password", &(radio_config->broadcast.password)) &&
        config_setting_lookup_bstring(broadcastsetting, "mount", &(radio_config->broadcast.mount)) && 
        config_setting_lookup_bstring(broadcastsetting, "name", &(radio_config->broadcast.name)) &&
        config_setting_lookup_bstring(broadcastsetting, "description", &(radio_config->broadcast.description)) &&
        config_setting_lookup_bstring(broadcastsetting, "genre", &(radio_config->broadcast.genre)) &&
        config_setting_lookup_bstring(broadcastsetting, "url", &(radio_config->broadcast.url)),
        "Could not read broadcast settings");

  if (cfg != NULL) config_destroy(cfg);
  return radio_config;
 error:
  if (cfg != NULL) config_destroy(cfg);
  return NULL;
};


