#ifndef MPD_H
#define MPD_H

#include <mpd/client.h>

int init_mpd_connection(void);
const char *get_current_playing(void);
unsigned get_song_duration(void);

extern struct mpd_connection *conn;
#endif
