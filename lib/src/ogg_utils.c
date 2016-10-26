
#include "vorbis/vorbisfile.h"

#include "ogg_utils.h"

#include "messages.h"

TrackInfo *get_track_info(OggVorbis_File *vf) {
  char **ptr = ov_comment(vf, -1)->user_comments;
  bstring artist;
  bstring title;
  while(*ptr) {
    if (strncmp(*ptr, "ARTIST", strlen("ARTIST")) == 0) {
      artist = bfromcstr( (*ptr + strlen("ARTIST=")) );
    } else if (strncmp(*ptr, "TITLE", strlen("TITLE")) == 0) {
      title = bfromcstr( (*ptr + strlen("TITLE=")) );
    }
    ++ptr;
  }
  TrackInfo *track_info = track_info_create(artist, title);
  return track_info;
}

int ov_channels(OggVorbis_File *vf) {
  vorbis_info *vi=ov_info(vf,-1);
  return vi->channels;
}
