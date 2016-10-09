#include <stdlib.h>
#include <stdio.h>

#include "virtual_ogg.h"

#include "bclib/dbg.h"

SF_VIRTUAL_IO *virtual_ogg_create() {
  SF_VIRTUAL_IO *sfvirt = malloc(sizeof(SF_VIRTUAL_IO));
  check_mem(sfvirt);

  sfvirt->get_filelen = &virt_get_filelen;
  sfvirt->seek = &virt_seek;
  sfvirt->read = &virt_read;
  sfvirt->write = &virt_write;
  sfvirt->tell = &virt_tell;

  return sfvirt;
 error:
  return NULL;
}

sf_count_t virt_get_filelen(void *user_data) {
  printf("Called file length\n");
  return 1000000;
}
sf_count_t virt_seek(sf_count_t offset, int whence, void *user_data) {
  printf("Called file seek\n");
  return 0;
}
sf_count_t virt_read(void *ptr, sf_count_t count, void *user_data) {
  printf("Called file read\n");
  return 0;
}
sf_count_t virt_write(const void *ptr, sf_count_t count, void *user_data) {
  printf("Called file write\n");
  return 0;
}
sf_count_t virt_tell(void *user_data) {
  printf("File tell\n");
  return 0;
}
