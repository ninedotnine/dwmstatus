/* 
 * 9.9
 * dan: compile with: gcc -Wall -pedantic -lX11 -lmpdclient -std=c99 dwm-status.c
 * consider replacing getAvgs() with a read from /proc/loadavg
 * make WARN_LOW_BATT_TEXT do something, the ""s make it tricky
 * colours!!!
 */

// this makes gcc not complain about implicit declarations of popen and pclose
// when using -pedantic and -std=c99. i guess that's good?
// also provides getloadavg and asprintf
// see: man 7 feature_test_macros
#define _GNU_SOURCE

// ideally this would only be defined if i knew zenity was installed
// but fuck that, right???? who even needs makefiles
#define zenity

// NDEBUG turns off all assert() calls
#define NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <mpd/client.h>
#include <getopt.h>
#include <stdbool.h>
#include "dwmstatus.h"
#include <netdb.h>
#include <errno.h>


#include "dwmstatus-defs.h"

const char * program_name;

bool noNetwork = false;

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

int getfiledata(const char *filename) {
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
    float temper = (float) getfiledata(TEMPERATURE) / 1000.0;
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
                char * const args[] = {"zenity", "--warning", 
                    "--text=you're a fat slut", NULL}; 
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

void net(char * (* const netOK)) {
    if (noNetwork) {
        int success = asprintf(netOK, "%s?%s", COLO_DEEPGREEN, COLO_RESET);
        if (success == -1){
            exit(15);
        }
        return;
    }
    struct addrinfo * info = NULL;
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
        if (asprintf(netOK, "%s%s%s", COLO_RED,
                                      gai_strerror(error),
                                      COLO_RESET) == -1) {
            fputs("error, cannot allocate mem\n", stderr);
            exit(12);
        }
    } else if (info == NULL) {
        if (asprintf(netOK, "could not getaddrinfo") == -1) {
            fputs("error, cannot allocate mem\n", stderr);
            exit(13);
        }
    } else {
        int sockfd = socket(info->ai_family,
                            info->ai_socktype,
                            info->ai_protocol);
        int success; // check the return value of asprintf
        if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
            int errnum = errno;
            fprintf(stderr, "errno is: %i\n", errnum);
            success = asprintf(netOK, "%sNET%s", COLO_YELLOW, COLO_RESET);
        } else {
            success = asprintf(netOK, "%sOK%s", COLO_DEEPGREEN, COLO_RESET);
        }
        close(sockfd);
        if (success == -1) {
            fputs("error, unable to malloc() in asprintf()", stderr);
            exit(14);
        }
    }
    freeaddrinfo(info);
}

void handle_mpc_error(struct mpd_connection *c) {
    assert(mpd_connection_get_error(c) != MPD_ERROR_SUCCESS);
    fprintf(stderr, "%s\n", mpd_connection_get_error_message(c));
    mpd_connection_free(c);
}

void getNowPlaying(char * (* const string)) {
    struct mpd_connection *conn = mpd_connection_new(NULL, 0, 0);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        handle_mpc_error(conn);
        return;
    }

	struct mpd_status * status = mpd_run_status(conn);
    struct mpd_song *song = mpd_run_current_song(conn);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        handle_mpc_error(conn);
        *string = calloc(1, 0);
        return;
    }

    if (mpd_status_get_state(status) != MPD_STATE_PLAY) {
        return;
    }

    unsigned pos = mpd_song_get_pos(song); 
    unsigned leng = mpd_status_get_queue_length(status);

    const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    const char *artist = mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0);

    // if no composer tag, try "artist" tag instead
    if (artist == NULL) {
        artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);

        // if no artist tag, use filename
        if (artist == NULL) {
			artist = mpd_song_get_uri(song);
        }
    }

// FIXME: improve this gross code
    int success; 
    // make title blue, artist magenta 
    if (title != NULL && artist != NULL) {
        success = asprintf(string, "%u/%u: %s%s%s - %s%s ♫%s ", pos, leng,
                           COLO_BLUE, title, COLO_RESET, COLO_MAGENTA, artist, 
                           COLO_RESET);
    } else if (artist == NULL) {
        success = asprintf(string, "%u/%u: %s%s %s♫%s ", pos, leng, COLO_BLUE,
                           title, COLO_MAGENTA, COLO_RESET);
    } else {
        success = asprintf(string, "%u/%u: %s%s %s♫%s ", pos, leng, COLO_BLUE,
                           artist, COLO_MAGENTA, COLO_RESET);
    }
    if (success == -1) {
        fputs("error, unable to malloc() in asprintf()", stderr);
        exit(18);
    }

    mpd_song_free(song);
    mpd_status_free(status);

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS ||
            !mpd_response_finish(conn)) {
        handle_mpc_error(conn);
        return;
    }

    mpd_connection_free(conn);
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
    fputs(" -r --report       report status immediately (default)\n", stream);
    fputs(" -n --no-netctork  skip network check\n", stream);
    exit(exit_code);
}

int main(int argc, char * argv[]) {
    program_name = argv[0]; // global

    /* list available args */
    const char * const shortOptions = "hdrun";
    const struct option longOptions[] = {
        {"help", 0, NULL, 'h'},
        {"daemon", 0, NULL, 'd'},
        {"report", 0, NULL, 'r'},
        {"update", 0, NULL, 'u'},
        {"no-network", 0, NULL, 'n'},
    };

    /* parse args */
    int nextOption;
    bool daemonMode = false;
    bool updateOnce = false;

    do {
        nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);
        switch (nextOption) {
            case 'h':
                usage(stdout, 0);
            case 'd':
                daemonMode = true;
                break;
            case 'r':
                daemonMode = false;
                updateOnce = false;
                break;
            case 'u':
                updateOnce = true;
                break;
            case 'n':
                noNetwork = true;
                break;
            case '?':
                usage(stderr, 1);
            case (-1): 
                break;
            default:
                // when does this happen? 
                printf("what? exiting\n");
                exit(EXIT_FAILURE);
        }
    } while (nextOption != -1);

    static Display *dpy;

    if (daemonMode || updateOnce) {
        if (!(dpy = XOpenDisplay(NULL))) {
            fputs("Cannot open display. are you _sure_ X11 is running?\n",
                    stderr);
            return EXIT_FAILURE;
        }
        if (daemonMode) {
            daemon(0, 0);
            for (; ; sleep(SLEEP_INTERVAL)) {
                setStatus(dpy);
            }
        } else {
            setStatus(dpy);
        }
    } else {
        puts(buildStatus());
    }
    return 0;
}

void setStatus(Display *dpy) {
    char * status = buildStatus();
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), status);
    XSync(dpy, False);
    free(status);
}

char * buildStatus(void) {
    // you need to call free() after calling this function!
    char * status;
    static double avgs[3];
    static char * batt;
    static char * temper;
    static char * netOK;
    static char * time;
    static char * nowPlaying;

    getAvgs(&avgs);
    getBattery(&batt);
    getTemperature(&temper);
    net(&netOK);
    getdatetime(&time);
    getNowPlaying(&nowPlaying);
    // thank you, _GNU_SOURCE, for asprintf
    // asprintf returns -1 on error, we check for that 
    if (asprintf(&status, OUTFORMAT,
                 nowPlaying,
                 avgs[0], avgs[1], avgs[2],
                 batt,
                 temper,
                 netOK, time) == -1) {
        fputs("error, unable to malloc() in asprintf()", stderr);
        exit(EXIT_FAILURE);
    }
    free(batt);
    free(temper);
    free(netOK);
    free(time);
    free(nowPlaying);
    return status;
}
