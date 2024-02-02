#ifndef MPD_H
#define MPD_H

#include <mpd/client.h>

int init_mpd_connection(void);
char *get_current_playing(void);
char *get_volume_str(void);
bool get_song_position_on_duration(unsigned *elaps, unsigned *dur);
unsigned get_song_duration(void);

extern struct mpd_connection *conn;
#endif
