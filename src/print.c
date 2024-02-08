#include <string.h>
#include <stdlib.h>

#include <mpd/client.h>

#include "ui.h"
#include "main.h"
#include "client_mpd.h"

static bool print_none(void)
{
    ui_clear_scrn();
    return true;
}

static bool print_queue(void)
{
    struct mpd_song **queue = get_song_queue();
    if (!queue) {
        ui_print_error(COL_ERR, "Unable to get queue");
        return false;
    }

    char *song_title = NULL;
    char *song_artist = NULL ;

    char *current_song_title = getstr_current_playing(MPD_TAG_TITLE);

    ui_print_str(true, "\r");

    for (size_t i = 0; queue[i] != NULL; i++) {
        song_title = getstr_song_info(queue[i], MPD_TAG_TITLE);
        song_artist = getstr_song_info(queue[i], MPD_TAG_ARTIST);

        if (!song_title || !song_artist) {
            ui_print_error(COL_ERR, "Unable to get song info");
            return false;
        }

        char *song_title_c = utf8_to_cstr(song_title);
        char *song_artist_c = utf8_to_cstr(song_artist);

        if (current_song_title && !strcmp(current_song_title, song_title)) {
            wattron(display.win_ui, COLOR_PAIR(COL_UI_ALT));
            ui_print_str(false, "%s", song_title_c);
            wattroff(display.win_ui, COLOR_PAIR(COL_UI_ALT));
        } else {
            wattron(display.win_ui, COLOR_PAIR(COL_UI));
            ui_print_str(false, "%s", song_title_c);
            wattroff(display.win_ui, COLOR_PAIR(COL_UI));
        }


        free(song_title);
        free(song_artist);
        free(song_title_c);
        free(song_artist_c);
    }
    free(queue);

    return true;
}


bool (*print_func[])(void) = {
    print_none,
    print_queue,
};

void print_run_print_state(enum Print_State STATE)
{
    print_func[STATE]();
}
