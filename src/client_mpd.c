#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>
#include <mpd/client.h>

#include "client_mpd.h"
#include "ui.h"
#include "main.h"

struct mpd_connection *conn = NULL;

char *utf8_to_cstr(const char *utf8_str)
{
    size_t utf8_len = strlen(utf8_str);

    char *cstr = malloc(utf8_len + 1);
    if (!cstr)
        return NULL;

    strncpy(cstr, utf8_str, utf8_len);
    cstr[utf8_len] = '\0';
    return cstr;
}

static struct mpd_status *get_status(void)
{
    mpd_command_list_begin(conn, true);  
    {
        mpd_send_status(conn);                
        mpd_send_current_song(conn);          
    }
    mpd_command_list_end(conn);           

    return mpd_recv_status(conn);
}

static struct mpd_song *get_song(void)
{
    struct mpd_status *status;
    struct mpd_song *song;
    enum mpd_state state;

    status = get_status();
    state = mpd_status_get_state(status);

    if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
        mpd_response_next(conn);
        song = mpd_recv_song(conn); 
        mpd_response_finish(conn);
        mpd_status_free(status);
        return song;
    }

    mpd_status_free(status);
    mpd_response_finish(conn);
    return NULL;
}

char *getstr_song_info(struct mpd_song *song, enum mpd_tag_type tag)
{
    const char *s = NULL;
    if (song) {
        if ((s = mpd_song_get_tag(song, tag, 0))) {
            s = strdup(mpd_song_get_tag(song, tag, 0));
        }
        mpd_response_finish(conn);
    }

    return (char *)s;
}

char *getstr_current_playing(enum mpd_tag_type tag)
{
    struct mpd_song *song;
    const char *s = NULL;
    song = get_song();

    if (song) {
        s = getstr_song_info(song, tag);
        mpd_song_free(song);
    }

    return (char *)s;
}

void queue_free(struct mpd_song **q)
{
    for (size_t i = 0; q[i] != NULL; i++) {
        mpd_song_free(q[i]);
    }
}

struct mpd_song **get_song_queue(void)
{
    struct mpd_song **song_arr = NULL;
    struct mpd_entity *entity;

    if (!mpd_send_list_queue_meta(conn)) {
        return NULL;
    }

    size_t i = 0;
    while ((entity = mpd_recv_entity(conn))) {
        song_arr = realloc(song_arr, sizeof(struct mpd_song *) * (i+2)); // One extra for NULL

        const struct mpd_song *song;
        song = mpd_entity_get_song(entity);
        song_arr[i++] = mpd_song_dup(song);

        mpd_entity_free(entity);
    }

    mpd_response_finish(conn);
    if (!i) {
        return NULL;
    }

    song_arr[i] = NULL;
    return song_arr;
}

bool get_song_position_on_duration(unsigned *elaps, unsigned *dur)
{
    bool ret = false;
    struct mpd_song *song = get_song();
    struct mpd_status *status = get_status();

    if (song && status) {
        if (dur) {
            *dur = mpd_song_get_duration(song);
        }
        if (elaps) {
            *elaps = mpd_status_get_elapsed_time(status);
        }
        ret = true;
    }


    if (song) mpd_song_free(song);
    if (status) mpd_status_free(status);
    mpd_response_finish(conn);

    return ret;
}

char *getstr_volume(void)
{
    int volume_perc = mpd_run_get_volume(conn);
    if (volume_perc < 0) {
        ui_print_error(COL_ERR, "Unable to get volume!");
        return NULL;
    }

    char s[16];
    sprintf(s, "%d%%", volume_perc);

    return strdup(s);
}



int init_mpd_connection(void)
{
    conn = mpd_connection_new(NULL, 0, 0);
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        exit_clean(1, conn);
    }

    return 0;
}
