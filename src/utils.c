#include "utils.h"

#include "config.h"
#include "music.h"
#include "net.h"

#include <assert.h>
#include <iso646.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

static Display *dpy = NULL;
static pthread_mutex_t x11_mutex = PTHREAD_MUTEX_INITIALIZER;

void open_x11(void) {
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fputs("Cannot open display. are you _sure_ X11 is running?\n", stderr);
        exit(EXIT_FAILURE);
    }
}

static void get_time(char buffer[static TIME_STR_LEN]) {
    time_t cur_time = time(NULL);
    struct tm *cur_tm = localtime(&cur_time);

    if (cur_tm == NULL) {
        fputs("Error getting localtime.\n", stderr);
        snprintf(buffer, TIME_STR_LEN, COLO_RED "???" COLO_RESET);
        return;
    }
    size_t length = strftime(buffer, TIME_STR_LEN, TIME_STR_FMT, cur_tm);
    if (length < 1) {
        fputs("strftime is 0.\n", stderr);
        snprintf(buffer, TIME_STR_LEN, COLO_RED "???" COLO_RESET);
    }
}

static int read_int_from_file(const char * const filename) {
    // this function parses an int from filename
    FILE *fd = fopen(filename, "r");
    int result;
    if (fd == NULL) {
        char time_buf[TIME_STR_LEN];
        get_time(time_buf);
        fprintf(stderr, "error opening %s at %s\n", filename, time_buf);
        return -1;
    }
    int length = fscanf(fd, "%d", &result);
    fclose(fd);
    if (length < 1) {
        fprintf(stderr, "trouble reading int from %s\n", filename);
    }
    return result;
}

static void get_temperature(char buffer[static TEMPERATURE_STR_LEN]) {
    double temper = read_int_from_file(TEMPERATURE) / 1000.0;
    char * colo;
    if (temper > 85) {
        colo = COLO_RED;
    } else if (temper > 75) {
        colo = COLO_YELLOW;
    } else if (temper < 60) {
        colo = COLO_CYAN;
    } else {
        colo = ""; // empty string for no colour
    }
    snprintf(buffer, TEMPERATURE_STR_LEN, "%s"TEMPERATURE_STR_FMT"%s",
                                          colo,    temper,    COLO_RESET);
}

static void get_batt(char buffer[static BATT_STR_LEN]) {
    bool chargin = false;
    int capacity = read_int_from_file(BATT_CAPACITY);
    if (capacity < 95) {
        FILE *fd;
        fd = fopen(BATT_STATUS, "r");
        if (fd == NULL) {
            fprintf(stderr, "Error opening BATT_STATUS. %s\n", BATT_STATUS);
            snprintf(buffer, BATT_STR_LEN, "%s?%d%%%s", COLO_RED, capacity, COLO_RESET);
            return;
        }

        char status[16] = {0};
        char * fgets_ret = fgets(status, 15, fd);
        fclose(fd);
        if (fgets_ret == NULL) {
            fprintf(stderr, "error reading BATT_STATUS: %s\n", BATT_STATUS);
            snprintf(buffer, BATT_STR_LEN, "%s%d%%?%s", COLO_RED, capacity, COLO_RESET);
            return;
        }

        chargin = (0 != strncmp(status, "Discharging", 11));
        if (not chargin and capacity <= WARN_LOW_BATT) {
            fputs("low battery warning\n", stderr);
#ifdef ZENITY
            // display a warning on low battery and not plugged in.
            if (not fork()) {
                char * const args[] = {"env", "zenity", "--warning",
                            "--width=600", "--text=" WARN_LOW_BATT_TEXT, NULL};
                execv("/usr/bin/env", args);
            }
#endif
        }
    }

    // colourize the result
    char * colo;
    if (chargin) {
        colo = COLO_MAGENTA;
    } else if (capacity > 50) {
        colo = COLO_DEEPGREEN;
    } else if (capacity > 30) {
        colo = COLO_CYAN;
    } else if (capacity > WARN_LOW_BATT) {
        colo = COLO_YELLOW;
    } else {
        colo = COLO_RED;
    }

    snprintf(buffer, BATT_STR_LEN, "%s%i%%%s", colo, capacity, COLO_RESET);
}

static void get_avgs(double avgs[static 3]) {
    // is this dumb?
    // you could just read from /proc/loadavg instead...
    // i dunno, the system call is probably just as good...
    int num_avgs = getloadavg(avgs, 3);
    assert (num_avgs == 3);
    if (num_avgs < 3) {
        if (num_avgs == -1) {
            fputs("num_avgs is -1\n", stderr);
            num_avgs = 0;
        }
        for (int i = num_avgs; i < 3; i++) {
            avgs[i] = 9.9;
        }
    }
}

void set_status_to(const char * const string) {
    int lock_ret = pthread_mutex_lock(&x11_mutex);
    assert (lock_ret == 0);
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), string);
    XSync(dpy, False);
    lock_ret = pthread_mutex_unlock(&x11_mutex);
    assert (lock_ret == 0);
}

void set_status(const char * const net_str) {
    char status_buf[EVERYTHING_STR_LEN];
    build_status(net_str, status_buf);
    set_status_to(status_buf);
}

void build_status(const char * const net_str, char * everything_buf) {
    double avgs[3];
    char batt_buf[BATT_STR_LEN];
    char temperature_buf[TEMPERATURE_STR_LEN];
    char time_buf[TIME_STR_LEN];
    char music_buf[MPD_STR_LEN];

    get_avgs(avgs);
    get_batt(batt_buf);
    get_temperature(temperature_buf);
    get_time(time_buf);
    get_now_playing(music_buf); // this might set nowPlaying to NULL

    int lock_ret = pthread_mutex_lock(&net_buf_mutex);
    assert(lock_ret == 0);
    int len = snprintf(everything_buf, EVERYTHING_STR_LEN, EVERYTHING_STR_FMT,
                       music_buf, avgs[0], avgs[1], avgs[2], batt_buf,
                       temperature_buf, net_str, time_buf);
    if (len < 1) {
        fputs("error, snprintf len < 1", stderr);
    } else if (len >= EVERYTHING_STR_LEN) {
        fprintf(stderr, "warning, snprintf output truncated, wanted %d\n", len);
    }
    lock_ret = pthread_mutex_unlock(&net_buf_mutex);
    assert(lock_ret == 0);
}
