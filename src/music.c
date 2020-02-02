#include "music.h"
#include "dwmstatus-defs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t music_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_mpd_error(struct mpd_connection *c) {
    assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);
    fprintf(stderr, "mpd: %s\n", mpd_connection_get_error_message(c));
    mpd_connection_free(c);
}

struct mpd_connection * establish_mpd_conn(void) {
    int success = pthread_mutex_lock(&music_mutex);
    assert (success == 0);
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);

    while (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "mpd_idler: ");
        handle_mpd_error(conn);
        success = pthread_mutex_unlock(&music_mutex);
        assert (success == 0);
        sleep(15);
        success = pthread_mutex_lock(&music_mutex);
        assert (success == 0);
        conn = mpd_connection_new(NULL, 0, 10000);
    }

    success = pthread_mutex_unlock(&music_mutex);
    assert (success == 0);

    return conn;
}


void getNowPlaying(char * (* const string)) {
    // this function sets the input to NULL if mpd connection fails
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "handling error 1\n");
        handle_mpd_error(conn);
        *string = NULL;
        return;
    }

	struct mpd_status * status = mpd_run_status(conn);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "handling error 2\n");
        handle_mpd_error(conn);
        *string = NULL;
        mpd_status_free(status);
        return;
    }

    struct mpd_song *song = mpd_run_current_song(conn);

    if (song == NULL || mpd_status_get_state(status) != MPD_STATE_PLAY) {
        *string = NULL;
        if (song != NULL) {
            mpd_song_free(song);
        }
        mpd_status_free(status);
        mpd_connection_free(conn);
        return;
    }

    unsigned pos = mpd_song_get_pos(song) + 1; // queue is 0-indexed
    unsigned leng = mpd_status_get_queue_length(status);

    const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    const char *artist = mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0);

    // if no composer tag, try "artist" tag instead
    if (artist == NULL) {
        artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);

        // if no artist tag, use filename. this should never return null
        if (artist == NULL) {
			artist = mpd_song_get_uri(song);
        }
    }
    assert(artist != NULL);

    int success;
    if (title == NULL) {
        success = asprintf(string, "%u/%u: %s%s %s♫%s ", pos, leng, COLO_BLUE,
                           artist, COLO_MAGENTA, COLO_RESET);
    } else {
        // make title blue, artist magenta
        success = asprintf(string, "%u/%u: %s%s%s - %s%s ♫%s ", pos, leng,
                           COLO_BLUE, title, COLO_RESET, COLO_MAGENTA, artist,
                           COLO_RESET);
    }
    if (success == -1) {
        fprintf(stderr, "handling error 4\n");
        fputs("error, unable to malloc() in asprintf()", stderr);
        exit(18);
    }

    mpd_song_free(song);
    mpd_status_free(status);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
            !mpd_response_finish(conn)) {
        fprintf(stderr, "handling error 5\n");
        handle_mpd_error(conn);
        return;
    }

    mpd_connection_free(conn);
}
