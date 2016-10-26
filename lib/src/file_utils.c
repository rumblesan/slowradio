
#include <stdlib.h>
#include <stdbool.h>
#include <glob.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "file_utils.h"

#include "bclib/bstrlib.h"
#include "bclib/list.h"
#include "bclib/dbg.h"

bool is_regular_file(const char *path) {
  struct stat path_stat;
  stat(path, &path_stat);
  return S_ISREG(path_stat.st_mode);
}

bstring get_random_file(bstring pattern) {
  glob_t globbuf;

  List *filelist = list_create();
  check(filelist != NULL, "Could not create file list");

  check(!glob(bdata(pattern), GLOB_NOSORT, NULL, &globbuf), "Could not glob folder");

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
