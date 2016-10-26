#ifndef __SLOW_RADIO_OGG_UTILS__
#define __SLOW_RADIO_OGG_UTILS__

#include <stdbool.h>

#include "bclib/bstrlib.h"
#include "vorbis/vorbisfile.h"
#include "messages.h"

TrackInfo *get_track_info(OggVorbis_File *vf);

int ov_channels(OggVorbis_File *vf);

#endif
