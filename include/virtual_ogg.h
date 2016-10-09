#ifndef __SLOW_RADIO_VIRTUAL_OGG__
#define __SLOW_RADIO_VIRTUAL_OGG__

#include <sndfile.h>

sf_count_t virt_get_filelen(void *user_data);
sf_count_t virt_seek(sf_count_t offset, int whence, void *user_data);
sf_count_t virt_read(void *prt, sf_count_t count, void *user_data);
sf_count_t virt_write(const void *prt, sf_count_t count, void *user_data);
sf_count_t virt_tell(void *user_data);

SF_VIRTUAL_IO *virtual_ogg_create();

void virtual_ogg_destroy(SF_VIRTUAL_IO *sfvirt);

#endif
