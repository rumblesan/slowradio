#include <stdlib.h>
#include <stdio.h>

#include "virtual_ogg.h"

#include "messages.h"

#include "bclib/dbg.h"
#include "bclib/ringbuffer.h"

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
  debug("Called file length");
  return 0;
}
sf_count_t virt_seek(sf_count_t offset, int whence, void *user_data) {
  debug("Called file seek");
  return 0;
}
sf_count_t virt_read(void *ptr, sf_count_t count, void *user_data) {
  debug("Called file read");
  return 0;
}
sf_count_t virt_write(const void *ptr, sf_count_t count, void *_rb) {
  RingBuffer *rb = _rb;
  if (rb_full(rb)) {
    log_err("Virtual Ogg: Could not write to output buffer");
    return 0;
  }

  unsigned char *data = malloc(count * sizeof(unsigned char *));
  check_mem(data);
  memcpy(data, ptr, (size_t) count) ;
  Message *msg = file_chunk_message(data, count);
  check(msg != NULL, "Virtual Ogg: Could not create file chunk message");
  rb_push(rb, msg);
  return count;
 error:
  return 0;
}
sf_count_t virt_tell(void *user_data) {
  debug("Called file tell");
  return 0;
}

void virtual_ogg_destroy(SF_VIRTUAL_IO *sfvirt) {
  free(sfvirt);
}
