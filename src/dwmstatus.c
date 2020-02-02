/*
 * 9.9
 * consider replacing getAvgs() with a read from /proc/loadavg
 * colours!!!
 */

// ideally this would only be defined if i knew zenity was installed
// but fuck that, right???? who even needs makefiles
#define zenity

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
#include <stdbool.h>
#include <pthread.h>

/* global variables */
static const char * program_name;
static Display *dpy;
static pthread_mutex_t x11_mutex = PTHREAD_MUTEX_INITIALIZER;

void * mpd_idler(__attribute__((unused)) void * arg) {
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
        setStatus();
    }
}

void getdatetime(char * (* const input)) {
    time_t result;
    struct tm *resulttm;

    if (((*input) = calloc(65, sizeof(char))) == NULL) {
        fputs("Cannot allocate memory for buf.\n", stderr);
        exit(10);
    }
    result = time(NULL);
    resulttm = localtime(&result);
    if (resulttm == NULL) {
        fputs("Error getting localtime.\n", stderr);
        sprintf((*input), "time ???");
    }
    if (! strftime((*input), sizeof(char)*65-1, TIMESTRING, resulttm)) {
        fputs("strftime is 0.\n", stderr);
        sprintf((*input), "time ????");
    }
}

int getfiledata(const char * const filename) {
    // this function parses an int from filename
    FILE *fd;
    int result;
    fd = fopen(filename, "r");
    if (fd == NULL) {
        fputs("error in getfiledata()\n", stderr);
        fprintf(stderr, "filename is %s\n", filename);
        char ** dumb = NULL;
        getdatetime(dumb);
        fprintf(stderr, "time: %s\n", (*dumb));
        return -1;
    }
    fscanf(fd, "%d", &result);
    fclose(fd);
    return result;
}

void getTemperature(char * (* const result)) {
    double temper = getfiledata(TEMPERATURE) / 1000.0;
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
    asprintf(result, "%s%02.1f°C%s", colo, temper, COLO_RESET);
}

void getBattery(char * (* const batt)) {
    int capacity;
    bool chargin = false;
    capacity = getfiledata(BATT_CAPACITY);
    if (capacity < 95) {
        FILE *fd;
        fd = fopen(BATT_STATUS, "r");
        if (fd == NULL) {
            fputs("Error opening BATT_STATUS.\n", stderr);
            asprintf(batt, "%s? %d%%%s", COLO_RED, capacity, COLO_RESET);
            return;
        }

        char * status;
        if ((status = calloc(15, sizeof(char))) == NULL) {
            exit(11);
        }
        fgets(status, 15, fd);
        fclose(fd);

        chargin = (0 != strncmp(status, "Discharging", 11));
        free(status);
        if (! chargin && capacity <= WARN_LOW_BATT) {
            fputs("low battery warning\n", stderr);
#ifdef zenity
            // display a warning on low battery and not plugged in.
            // depends on zenity
            if (! fork()) {
                char * const args[] = {"zenity", "--warning", "--width=600",
                    "--text=" WARN_LOW_BATT_TEXT, NULL};
                execv("/usr/bin/zenity", args);
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

    if (asprintf(batt, "%s%i%%%s", colo, capacity, COLO_RESET) == -1) {
        fputs("whoooaaa, test in getBattery was -1", stderr);
        exit(EXIT_FAILURE);
    }
}


void getAvgs(double (* avgs)[3]) {
    // is this dumb?
    // you could just read from /proc/loadavg instead...
    // i dunno, the system call is probably just as good...
    int num_avgs = getloadavg(*avgs, 3);
    assert (num_avgs == 3);
    if (num_avgs < 3) {
        if (num_avgs == -1) {
            fputs("num_avgs is -1\n", stderr);
            num_avgs = 0;
        }
        for (int i = num_avgs; i < 3; i++) {
            (* avgs)[i] = 9.9;
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
    int success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s?%s", COLO_DEEPGREEN, COLO_RESET);
    if (success > MAX_NET_MSG_LEN) {
        fputs("the net buffer is too small.\n", stderr);
    } else if (success < 1) {
        fputs("error, problem with snprintf\n", stderr);
        exit(16);
    }

    if (! noNetwork) {
        if (update_mode || report_mode) {
            update_net_buffer();
        }
        if (daemon_mode) {
            pthread_t network_worker;
            pthread_create(&network_worker, NULL, network_updater, NULL);
        }
    }

    if (daemon_mode || update_mode) {
        if (!(dpy = XOpenDisplay(NULL))) {
            fputs("Cannot open display. are you _sure_ X11 is running?\n",
                    stderr);
            return EXIT_FAILURE;
        }
    }

    if (daemon_mode) {
//         if (! run_in_foreground) {
//             if (be_quiet) {
//                 daemon(0, 0);
//             } else {
//                 daemon(0, 1);
//             }
//         }
        daemon(0, 1);

        pthread_t idler;
        pthread_create(&idler, NULL, mpd_idler, NULL);

        while (true) {
            setStatus();
            sleep(SLEEP_INTERVAL);
        }
    }

    if (update_mode || report_mode) {
        char * status = buildStatus();
        if (update_mode) {
            set_status_to(status);
        }
        if (report_mode) {
            puts(status);
        }
        free(status);
    }
    return 0;
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


void setStatus(void) {
    int success = pthread_mutex_lock(&x11_mutex);
    assert (success == 0);
    char * status = buildStatus();
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), status);
    XSync(dpy, False);
    free(status);
    success = pthread_mutex_unlock(&x11_mutex);
    assert (success == 0);
}

char * buildStatus(void) {
    // you need to call free() after calling this function!
    char * status;
    static double avgs[3];
    static char * batt;
    static char * temper;
    static char * time;
    static char * nowPlaying;

    getAvgs(&avgs);
    getBattery(&batt);
    getTemperature(&temper);
    getdatetime(&time);
    getNowPlaying(&nowPlaying); // this might set nowPlaying to NULL

    int success = pthread_mutex_lock(&net_buf_mutex);
    assert(success == 0);
    // thank you, _GNU_SOURCE, for asprintf
    // asprintf returns -1 on error, we check for that
    if (asprintf(&status, OUTFORMAT,
                 (nowPlaying ? nowPlaying : ""),
                 avgs[0], avgs[1], avgs[2],
                 batt,
                 temper,
                 net_buf, time) == -1) {
        fputs("error, unable to malloc() in asprintf()", stderr);
        exit(EXIT_FAILURE);
    }
    success = pthread_mutex_unlock(&net_buf_mutex);
    assert(success == 0);

    free(batt);
    free(temper);
    free(time);
    free(nowPlaying); // If ptr is NULL, no operation is performed.
    return status;
}
