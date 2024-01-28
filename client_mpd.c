#include <ncurses.h>
#include <string.h>
#include <mpd/client.h>
#include <unistd.h>

#include "client_mpd.h"
#include "main.h"

struct mpd_connection *conn = NULL;


const char *fget_current_playing(void)
{
    mpd_command_list_begin(conn, true);

    mpd_send_status(conn);
    mpd_send_current_song(conn);
    mpd_command_list_end(conn);

    struct mpd_status *status = mpd_recv_status(conn);
    enum mpd_state state = mpd_status_get_state(status);
    if (state == MPD_STATE_PLAY || state == MPD_STATE_PAUSE) {
        mpd_response_next(conn);
        struct mpd_song *song = mpd_recv_song(conn);

        char *s = strdup(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
        mpd_song_free(song);
        mpd_status_free(status);
        mpd_response_finish(conn);
        return s;
    }

    mpd_response_finish(conn);

    return NULL;
}

struct mpd_song *get_song(void)
{
    struct mpd_status *status;
    struct mpd_song *song;
    enum mpd_state state;

    mpd_command_list_begin(conn, true);
    {
        mpd_send_status(conn);
        mpd_send_current_song(conn);
    }
    mpd_command_list_end(conn);

    status = mpd_recv_status(conn);
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


const char *get_current_playing(void)
{
    struct mpd_song *song;
    char *s = NULL;
    song = get_song();
    if (song) {
        s = strdup(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
        mpd_song_free(song);
        mpd_response_finish(conn);
    }

    return s;
}

int init_mpd_connection(void)
{
    conn = mpd_connection_new(NULL, 0, 0);
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        exit_clean(1, conn);
    }

    return 0;
}
