#ifndef __SLOW_RADIO_LOGGING__
#define __SLOW_RADIO_LOGGING__

#include <stdio.h>

char *timenow();

#define logger(P, M, ...) fprintf(stderr, "%s -  INFO: [%s] " M "\n", timenow(), P, ##__VA_ARGS__)

#define err_logger(P, M, ...) fprintf(stderr, "%s - ERROR: [%s] " M "\n", timenow(), P, ##__VA_ARGS__)

#endif
