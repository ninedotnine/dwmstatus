/* 
 * 9.9
 * dan: compile with: gcc -Wall -pedantic -lX11 -std=c99 dwm-status.c
 * last updated jan 10 2015
 * also, replace getAvgs() with a read from /proc/loadavg
 *    uhh, if you feel like it...
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
#include <getopt.h>
#include <stdbool.h>
#include "dwmstatus.h"
#include <netdb.h>
// #include <sys/types.h>
// #include <sys/socket.h>

#include "dwmstatus-defs.h"

#define COLO_RESET "\x1b[0m" // reset 
#define COLO_RED "\x1b[38;5;196m" // red
#define COLO_YELLOW "\x1b[38;5;190m" // yellow
#define COLO_DEEPGREEN "\x1b[38;5;34m" // deep green
#define COLO_MAGENTA "\x1b[38;5;199m" // magenta
#define COLO_BRIGHTGREEN "\x1b[38;5;46m" // bright green
#define COLO_CYAN "\x1b[38;5;45m" // bright blue
#define COLO_BLUE "\x1b[38;5;21m" // blue

const char * program_name;

char *getdatetime(void) {
    char *buf;
    time_t result;
    struct tm *resulttm;

    if ((buf = malloc(65 * sizeof(char))) == NULL) {
        fputs("Cannot allocate memory for buf.\n", stderr);
        return "time ???";
    }
    result = time(NULL);
    resulttm = localtime(&result);
    if (resulttm == NULL) {
        fputs("Error getting localtime.\n", stderr);
        return "time ???";
    }
    if (! strftime(buf, sizeof(char)*65-1, TIMESTRING, resulttm)) {
        fputs("strftime is 0.\n", stderr);
        return "time ???";
    }
    return buf;
}

int getfiledata(const char *filename) {
    // this function parses an int from filename 
    FILE *fd;
    int result; // returned
    fd = fopen(filename, "r");
    if (fd == NULL) {
        fputs("error in getfiledata()\n", stderr);
        fprintf(stderr, "filename is %s\n", filename);
        fprintf(stderr, "time: %s\n", getdatetime());
        return -1;
    }
    fscanf(fd, "%d", &result);
    fclose(fd);
    return result;
}

char * getTemperature(void) {
    float temper = (float) getfiledata(TEMPERATURE) / 1000.0;
    char * result; // will be returned
    char * colo; // for colouring
    if (temper > 85) {
        colo = COLO_RED;
    } else if (temper > 75) {
        colo = COLO_YELLOW;
    } else if (temper < 60) {
        colo = COLO_CYAN;
    } else {
        colo = ""; // empty string 
    }
    asprintf(&result, "%s%02.1f°C%s", colo, temper, COLO_RESET);
    return result;
}

char * getBattery(void) {
    int capacity;
    bool chargin = false;
    capacity = getfiledata(BATT_CAPACITY);
    if (capacity < 95) {
        FILE *fd;
        fd = fopen(BATT_STATUS, "r");
        if (fd == NULL) {
            fputs("Error opening BATT_STATUS.\n", stderr);
            return "??%";
        }

        char * status;
        status = malloc(15 * sizeof(char));
//         fscanf(fd, "%s", status);
        fgets(status, 15, fd);
        fclose(fd);

        chargin = (0 != strncmp(status, "Discharging", 11));
//         printf("chargin: %s\n", (chargin) ? "true" : "false");
//         if ( ! strncmp(status, "Discharging", 11) ) {
//         if ( ! chargin) {
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
    char * result; // will be returned
    char * colo; // colour code
    if (chargin) {
        colo = COLO_MAGENTA;
    } else if (capacity > 70) {
        colo = COLO_DEEPGREEN;
    } else if (capacity > 30) {
        colo = COLO_CYAN;
    } else if (capacity > WARN_LOW_BATT) {
        colo = COLO_YELLOW;
    } else {
        colo = COLO_RED;
    }

    if (asprintf(&result, "%s%i%%%s", colo, capacity, COLO_RESET) == -1) {
        fputs("whoooaaa, test in getBattery was -1", stderr);
        exit(EXIT_FAILURE);
    }
    return result;
}

char * net(void) {
    struct addrinfo * info = NULL;
    char * result; // will be returned
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
//         return "error";
//         result = "error";
        if (asprintf(&result, "error") == -1) {
            fputs("error, what do i even do now? oh no!\n", stderr);
            result = malloc(9 * sizeof(char));
            snprintf(result, 9, "%s", "soborked");
        }
    } else if (info == NULL) {
//         return "could not getaddrinfo";
//         result = "could not getaddrinfo";
        if (asprintf(&result, "could not getaddrinfo") == -1) {
            fputs("error, what do i even do now? aah!", stderr);
            result = malloc(9 * sizeof(char));
            snprintf(result, 9, "%s", "soborked");
        }
    } else {
//         puts("making socket\n");
        int sockfd = socket(info->ai_family, info->ai_socktype,info->ai_protocol);
//         puts("connecting \n");
        int success; // check the return value of asprintf
        if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
            success = asprintf(&result, "%sNET%s", COLO_RED, COLO_RESET);
        } else {
            success = asprintf(&result, "%sOK%s", COLO_DEEPGREEN, COLO_RESET);
        }
        if (success == -1) {
            fputs("error, unable to malloc() in asprintf()", stderr);
//             return "error";
            result = "soborked";
        }
    }
    return result;
}

double * getAvgs(void) {
    // you should call free after using this, methinks!
    // is this dumb?
    // you could just read from /proc/loadavg instead...
    // i dunno, the system call is probably just as good...
    // must return exactly 3 doubles 
    double * avgs = malloc(3 * sizeof(double));
//     double avgs[3];
//     double ** a;
//     a = &avgs;
//     puts("okay");
//     int num_avgs = getloadavg((*avgs), 3);
    int num_avgs = getloadavg(avgs, 3);
    assert (num_avgs == 3);
    if (num_avgs < 3) {
        if (num_avgs == -1) {
            fputs("num_avgs is -1", stderr);
            num_avgs = 0;
        } 
        for (int i = num_avgs; i < 3; i++) {
//             *avgs[i] = 9.9;
            avgs[i] = 9.9;
        }
    }
//     return *avgs;
    return avgs;
}

/*
char * getAvgs(void) {
    double avgs[3];
    if (getloadavg(avgs, 3) < 0) {
        perror("getloadavg broke");
        // return "oops";
        exit(EXIT_FAILURE);
    }
    return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}
*/

void usage(FILE * stream, int exit_code) {
    // prints what's up to stream, then quits with status exit_code
    fprintf(stream, "usage: %s [args]\n", program_name);
    fputs(" -h --help         display this message\n", stream);
    fputs(" -u --update       update the x root window once\n", stream);
    fputs(" -d --daemon       run in background\n", stream);
    fputs(" -r --report       report status immediately (default)\n", stream);
    exit(exit_code);
}

int main(int argc, char * argv[]) {
    program_name = argv[0]; // global

    /* list available args */
    const char * const shortOptions = "hdru";
    const struct option longOptions[] = {
        {"help", 0, NULL, 'h'},
        {"daemon", 0, NULL, 'd'},
        {"report", 0, NULL, 'r'},
        {"update", 0, NULL, 'u'},
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

    if (daemonMode) {
        if (!(dpy = XOpenDisplay(NULL))) {
            fputs("Cannot open display. are you _sure_ X11 is running?\n",
                   stderr);
            return EXIT_FAILURE;
        }
        daemon(0,0);
        for (; ; sleep(SLEEP_INTERVAL)) {
            setStatus(dpy);
        }
    } else {
        if (updateOnce) {
            if (!(dpy = XOpenDisplay(NULL))) {
                fputs("Cannot open display. Are you _sure_ X11 is running?\n",
                       stderr);
                return EXIT_FAILURE;
            }
            setStatus(dpy);
        } else {
            puts(buildStatus());
        }
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
    double * avgs = getAvgs();
    char * batt = getBattery();
    char * temper = getTemperature();
    char * netOK = net();
    char * time = getdatetime();
    // thank you, _GNU_SOURCE, for asprintf
    // asprintf returns -1 on error, we check for that 
    if (asprintf(&status, OUTFORMAT,
                 avgs[0], avgs[1], avgs[2],
                 batt,
                 temper,
                 netOK, time) == -1) {
        fputs("error, unable to malloc() in asprintf()", stderr);
        exit(EXIT_FAILURE);
    }
    free(avgs);
    free(batt);
    free(temper);
    free(netOK);
    free(time);
    return status;
}
