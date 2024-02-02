#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>
#include <mpd/client.h>

#include "client_mpd.h"
#include "main.h"

struct mpd_connection *conn = NULL;

    
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


char *get_current_playing(void)
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


bool get_song_position_on_duration(unsigned *elaps, unsigned *dur)
{
    struct mpd_song *song = get_song();
    struct mpd_status *status = get_status();

    if (!song || !status) {
        mpd_song_free(song);
        mpd_status_free(status);
        mpd_response_finish(conn);
        return false;
    }

    if (dur)
        *dur = mpd_song_get_duration(song);
    if (elaps)
        *elaps = mpd_status_get_elapsed_time(status);

    mpd_song_free(song);
    mpd_status_free(status);
    mpd_response_finish(conn);


    return true;
}

char *get_volume_str(void)
{
    int volume_perc = mpd_run_get_volume(conn);
    if (volume_perc < 0) {
        ui_print_error(COL_ERR, "Unable to get volume!");
    }

    char s[3];
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
