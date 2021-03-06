#include "music.h"

#include "config.h"
#include "utils.h"

#include <assert.h>
#include <iso646.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t mpd_conn_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_mpd_error(struct mpd_connection *c) {
    fprintf(stderr, "mpd: %s\n", mpd_connection_get_error_message(c));
}

struct mpd_connection * establish_mpd_conn(void) {
    int lock_ret = pthread_mutex_lock(&mpd_conn_mutex);
    assert (lock_ret == 0);
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);

    while (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "unable to establish mpd connection? ");
        handle_mpd_error(conn);
        mpd_connection_free(conn);
        lock_ret = pthread_mutex_unlock(&mpd_conn_mutex);
        assert (lock_ret == 0);
        sleep(15);
        lock_ret = pthread_mutex_lock(&mpd_conn_mutex);
        assert (lock_ret == 0);
        conn = mpd_connection_new(NULL, 0, 10000);
    }

    lock_ret = pthread_mutex_unlock(&mpd_conn_mutex);
    assert (lock_ret == 0);

    return conn;
}

void get_now_playing(char buffer[MPD_STR_LEN]) {
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "handling error 1\n");
        handle_mpd_error(conn);
        mpd_connection_free(conn);
        buffer[0] = '\0';
        return;
    }

	struct mpd_status * status = mpd_run_status(conn);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "handling error 2\n");
        mpd_status_free(status);
        handle_mpd_error(conn);
        mpd_connection_free(conn);
        buffer[0] = '\0';
        return;
    }

    struct mpd_song *song = mpd_run_current_song(conn);

    if (song == NULL or mpd_status_get_state(status) != MPD_STATE_PLAY) {
        // no song is playing.
        if (song != NULL) {
            mpd_song_free(song);
        }
        mpd_status_free(status);
        mpd_connection_free(conn);
        buffer[0] = '\0';
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

    int length;
    if (title == NULL) {
        length = snprintf(buffer, MPD_STR_LEN, "%u/%u: %s%s %s♫%s ", pos,
                          leng, COLO_BLUE, artist, COLO_MAGENTA, COLO_RESET);
    } else {
        // make title blue, artist magenta
        length = snprintf(buffer, MPD_STR_LEN, "%u/%u: %s%s%s - %s%s ♫%s ",
                          pos, leng, COLO_BLUE, title, COLO_RESET,
                          COLO_MAGENTA, artist, COLO_RESET);
    }
    if (length < -1) {
        fprintf(stderr, "error, snprintf should not return %d\n", length);
    } else if (length >= MPD_STR_LEN) {
        fprintf(stderr, "music buffer not long enough, wanted %d bytes\n", length);
    }

    mpd_song_free(song);
    mpd_status_free(status);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS or
            not mpd_response_finish(conn)) {
        fprintf(stderr, "handling error 5\n");
        handle_mpd_error(conn);
        mpd_connection_free(conn);
        return;
    }

    mpd_connection_free(conn);
}

void * mpd_idler(void * arg) {
    char * net_buf = arg;
    int detach_ret = pthread_detach(pthread_self());
    assert (detach_ret == 0);

    struct mpd_connection * conn = establish_mpd_conn();
    while (true) {
        enum mpd_idle event = mpd_run_idle(conn);
        if (event == 0) {
            fputs("error, mpd_run_idle broke. was mpd killed? ", stderr);
            mpd_connection_free(conn);
            conn = establish_mpd_conn();
        }
        set_status(net_buf);
    }
}
