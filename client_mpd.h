#ifndef MPD_H
#define MPD_H

#include <mpd/client.h>

int init_mpd_connection(void);
char *getstr_current_playing(enum mpd_tag_type tag);
char *getstr_song_info(struct mpd_song *song, enum mpd_tag_type tag);
char *getstr_volume(void);
char *utf8_to_cstr(const char *utf8_str);
struct mpd_song **get_song_queue(void);
bool get_song_position_on_duration(unsigned *elaps, unsigned *dur);
unsigned get_song_duration(void);

extern struct mpd_connection *conn;
#endif
