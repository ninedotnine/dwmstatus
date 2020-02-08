/*
 * 9.9
 * colours!!!
 */

// NDEBUG turns off all assert() calls
// #define NDEBUG

#include "music.h"
#include "net.h"
#include "dwmstatus.h"
#include "dwmstatus-defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <mpd/client.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>

/* global variables */
static const char * program_name;
static Display *dpy;
static pthread_mutex_t x11_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_signal(__attribute__((unused)) int sig) {
    // do nothing...
}

void * mpd_idler(void * arg) {
    char * net_buf = arg;
    int success = pthread_detach(pthread_self());
    assert (success == 0);

    struct mpd_connection * conn = establish_mpd_conn();
    while (true) {
        enum mpd_idle event = mpd_run_idle(conn);
        if (event == 0) {
            fputs("error, mpd_run_idle broke. was mpd killed? ", stderr);
            mpd_connection_free(conn);
            conn = establish_mpd_conn();
        }
        setStatus(net_buf);
    }
}

void get_time(char buffer[static TIME_STR_LEN]) {
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

int read_int_from_file(const char * const filename) {
    // this function parses an int from filename
    FILE *fd = fopen(filename, "r");
    int result;
    if (fd == NULL) {
        char time_buf[TIME_STR_LEN];
        get_time(time_buf);
        fprintf(stderr, "error opening %s at %s\n", filename, time_buf);
        return -1;
    }
    fscanf(fd, "%d", &result);
    fclose(fd);
    return result;
}

void get_temperature(char buffer[static TEMPERATURE_STR_LEN]) {
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

void get_batt(char buffer[static BATT_STR_LEN]) {
    bool chargin = false;
    int capacity = read_int_from_file(BATT_CAPACITY);
    if (capacity < 95) {
        FILE *fd;
        fd = fopen(BATT_STATUS, "r");
        if (fd == NULL) {
            fputs("Error opening BATT_STATUS.\n", stderr);
            snprintf(buffer, BATT_STR_LEN, "%s?%d%%%s", COLO_RED, capacity, COLO_RESET);
            return;
        }

        char status[16] = {0};
        fgets(status, 15, fd);
        fclose(fd);

        chargin = (0 != strncmp(status, "Discharging", 11));
        if (! chargin && capacity <= WARN_LOW_BATT) {
            fputs("low battery warning\n", stderr);
#ifdef ZENITY
            // display a warning on low battery and not plugged in.
            if (! fork()) {
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

void get_avgs(double avgs[static 3]) {
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

void usage(FILE * stream, int exit_code) {
    // prints what's up to stream, then quits with status exit_code
    fprintf(stream, "usage: %s [args]\n", program_name);
    fputs(" -h --help         display this message\n", stream);
    fputs(" -u --update       update the x root window once\n", stream);
    fputs(" -d --daemon       run in background\n", stream);
    fputs(" -f --foreground   like --daemon, but in foreground\n", stream);
    fputs(" -r --report       report status immediately (default)\n", stream);
    fputs(" -n --no-network   skip network check\n", stream);
    fputs(" -q --quiet        don't output to stdout or stderr\n", stream);
    fputs(" -v --version      output version and exit\n", stream);
    exit(exit_code);
}

int main(int argc, char * argv[]) {
    program_name = argv[0]; // global

    setlocale(LC_ALL, "");

    /* list available args */
    const char * const shortOptions = "hdfrunqv";
    const struct option longOptions[] = {
        {"help", 0, NULL, 'h'},
        {"daemon", 0, NULL, 'd'},
        {"foreground", 0, NULL, 'f'},
        {"report", 0, NULL, 'r'},
        {"update", 0, NULL, 'u'},
        {"no-network", 0, NULL, 'n'},
        {"quiet", 0, NULL, 'q'},
        {"version", 0, NULL, 'v'},
        {0, 0, 0, 0}, // this is necessary, apparently
    };

    /* parse args */
    int nextOption;
    bool be_quiet = false;
    bool run_in_foreground = false;
    bool daemon_mode = false;
    bool update_mode = false;
    bool report_mode = false;
    bool noNetwork = false;

    do {
        nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);
        switch (nextOption) {
            case 'h':
                usage(stdout, 0);
                break;
            case 'd':
                daemon_mode = true;
                update_mode = false;
                break;
            case 'f':
                daemon_mode = true;
                update_mode = false;
                run_in_foreground = true;
                break;
            case 'r':
                daemon_mode = false;
                report_mode = true;
                break;
            case 'u':
                update_mode = true;
                daemon_mode = false;
                break;
            case 'n':
                noNetwork = true;
                break;
            case 'q':
                be_quiet = true;
                break;
            case 'v':
                printf("dan's dwmstatus version: %s\n", VERSION);
                exit(EXIT_SUCCESS);
            case '?':
                usage(stderr, 1);
                break;
            case (-1):
                break;
            default:
                // when does this happen?
                printf("what? exiting\n");
                exit(EXIT_FAILURE);
        }
    } while (nextOption != -1);

    if (! (update_mode || daemon_mode || report_mode)) {
        report_mode = true; // a sensible default
    }

    // initialize the net_buf to a dummy string for now.
    // the buffer will be later populated by the network_worker,
    // unless noNetwork is true.
    char net_buf[MAX_NET_MSG_LEN];
    int success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s?%s", COLO_DEEPGREEN, COLO_RESET);
    if (success > MAX_NET_MSG_LEN) {
        fputs("the net buffer is too small.\n", stderr);
    } else if (success < 1) {
        fputs("error, problem with snprintf\n", stderr);
        exit(16);
    }

    if (daemon_mode || update_mode) {
        if (!(dpy = XOpenDisplay(NULL))) {
            fputs("Cannot open display. are you _sure_ X11 is running?\n",
                    stderr);
            return EXIT_FAILURE;
        }
    }

    if (daemon_mode) {
        if (! run_in_foreground) {
            if (be_quiet) {
                daemon(0, 0);
            } else {
                daemon(0, 1);
            }
        }
    }

    if (! noNetwork) {
        if (update_mode || report_mode) {
            update_net_buffer(net_buf);
        }
        if (daemon_mode) {
            pthread_t network_worker;
            pthread_create(&network_worker, NULL, network_updater, net_buf);
        }
    }

    if (daemon_mode) {
        pthread_t idler;
        pthread_create(&idler, NULL, mpd_idler, net_buf);

        /* set up signal handling. */
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = handle_signal;
        sa.sa_flags = SA_RESTART;
        success = sigaction(SIGUSR1, &sa, NULL);
        assert (success == 0);

        while (true) {
            setStatus(net_buf);
            sleep(SLEEP_INTERVAL);
        }
    } else {
        char status_buf[EVERYTHING_STR_LEN];
        build_status(net_buf, status_buf);
        if (update_mode) {
            set_status_to(status_buf);
        }
        if (report_mode) {
            puts(status_buf);
        }
        return 0;
    }
}

void set_status_to(const char * string) {
    int success = pthread_mutex_lock(&x11_mutex);
    assert (success == 0);
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), string);
    XSync(dpy, False);
    success = pthread_mutex_unlock(&x11_mutex);
    assert (success == 0);
}


void setStatus(char * net_buf) {
    char status_buf[EVERYTHING_STR_LEN];
    int success = pthread_mutex_lock(&x11_mutex);
    assert (success == 0);
    build_status(net_buf, status_buf);
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), status_buf);
    XSync(dpy, False);
    success = pthread_mutex_unlock(&x11_mutex);
    assert (success == 0);
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

    int success = pthread_mutex_lock(&net_buf_mutex);
    assert(success == 0);
    int len = snprintf(everything_buf, EVERYTHING_STR_LEN, EVERYTHING_STR_FMT,
                       music_buf, avgs[0], avgs[1], avgs[2], batt_buf,
                       temperature_buf, net_str, time_buf);
    if (len < 1) {
        fputs("error, snprintf len < 1", stderr);
    } else if (len >= EVERYTHING_STR_LEN) {
        fprintf(stderr, "warning, snprintf output truncated, wanted %d\n", len);
    }
    success = pthread_mutex_unlock(&net_buf_mutex);
    assert(success == 0);
}
