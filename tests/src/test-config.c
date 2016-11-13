#include "minunit.h"

#include "config.h"

char *test_config_read() {

  RadioInputCfg *cfg = NULL;
  cfg = read_config("../../radio.cfg");
  mu_assert(cfg != NULL, "Could not read config");

  destroy_config(cfg);
  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_config_read);

  return NULL;
}

RUN_TESTS(all_tests);
