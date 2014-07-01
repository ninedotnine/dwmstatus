/* 
 * 9.9
 * dan: compile with: gcc -Wall -pedantic -lX11 -std=c99 dwm-status.c
 * pass the argument -Dverbosity=2 when compiling for output
 * or -D experimental_alarm to see what happens (nothing useful)
 * last updated aug 16 2013
 * also, replace getavgs() with a read from /proc/loadavg
 */

// this makes gcc not complain about implicit declarations of popen and pclose
// when using -pedantic and -std=c99. i guess that's good?
// also provides getloadavg
// see: man 7 feature_test_macros
#define _BSD_SOURCE

/* SETTINGS */

// the format for the date/time
// const char timestring[] = "[%a %b %d] %H:%M";
#define TIMESTRING "[%a %b %d] %H:%M"
// the format for everything
// const char outformat[] = "[batt: %d%%] [mail %d] [pkg %d] [net %s] %s";
#define OUTFORMAT "[%.2f %.2f %.2f] [batt: %d%%] [mail %d] [pkg %d] [net %s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
// files -- populated by cron
#define MAILFILE "/tmp/dwm-status.mail"
#define PKGFILE "/tmp/dwm-status.packages"
// #define FBCMDFILE "/tmp/dwm-status.fbcmd"

// ideally this would only be defined if i knew zenity was installed
// but fuck that, right???? who even needs makefiles
#define zenity

// NDEBUG turns off all assert() calls
#define NDEBUG

#ifndef verbosity
#define verbosity 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <X11/Xlib.h>
// #include <netdb.h>

static void setstatus(const char *str, Display *dpy) {
    assert(dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

static char *getdatetime(void) {
#if verbosity > 1
    printf("entering getdatetime()\n");
#endif
    char *buf;
    time_t result;
    struct tm *resulttm;

    // if ((buf = malloc(sizeof(char)*65)) == NULL) {
    if ((buf = malloc(65)) == NULL) {
        fprintf(stderr, "Cannot allocate memory for buf.\n");
        return "time ???";
    }
    result = time(NULL);
    resulttm = localtime(&result);
    if (resulttm == NULL) {
        fprintf(stderr, "Error getting localtime.\n");
        return "time ???";
    }
    // if (! strftime(buf, sizeof(char)*65-1, TIMESTRING, resulttm)) {
    if (! strftime(buf, 65-1, TIMESTRING, resulttm)) {
        fprintf(stderr, "strftime is 0.\n");
        return "time ???";
    }
#if verbosity > 1
    printf("returning %s from getdatetime()\n", buf);
#endif
    return buf;
}

static int getfiledata(const char *filename) {
#if verbosity > 1
    printf("entering getfiledata(), arg is: %s\n", filename);
#endif
    FILE *fd;
    int result;
    fd = fopen(filename, "r");
    if (fd == NULL) {
        fprintf(stderr, "error in getfiledata()\n");
        fprintf(stderr, "filename is %s\n", filename);
        fprintf(stderr, "time: %s\n", getdatetime());
        return -1;
    }
    fscanf(fd, "%d", &result);
    fclose(fd);
#if verbosity > 1
    printf("returning %d from getfiledata()\n", result);
#endif
    return result;
}

static int getbattery(void) {
#if verbosity > 1
    printf("entering getbattery()\n");
#endif
    int capacity;
    capacity = getfiledata("/sys/class/power_supply/BAT0/capacity");

#ifdef experimental_alarm
    // alarm never seems to be anything but 0 ?_?
    int alarm;
    alarm = getfiledata("/sys/class/power_supply/BAT0/alarm");
    fprintf(stdout, "alarm is %d\n", alarm);
    if (alarm == 1) {
        fprintf(stderr, "low battery warning\n");
        system("zenity --warning --text='low battery!'"); 
    }
#endif

#ifdef zenity
    // display a warning on low battery and not plugged in. depends on zenity
    if (capacity <= WARN_LOW_BATT) {
        FILE *fd;
        fd = fopen("/sys/class/power_supply/BAT0/status", "r");
        if (fd == NULL) {
            fprintf(stderr, "Error opening status.\n");
            return -1;
        }
        char * status;
        status = malloc(15);
//         fscanf(fd, "%s", status);
        fgets(status, 15, fd);
        fclose(fd);
        if ( ! strncmp(status, "Discharging", 11) ) {
            fprintf(stderr, "low battery warning\n");
            if (fork()) { 
                char * const gayargs[] = {"zenity", "--warning", "--text=you're a fat slut", NULL};
//                 execv("zenity", {"--warning", "--text=what", NULL});
                execv("/usr/bin/zenity", gayargs);
                    /*
                system("zenity --warning \
                        --text=\"charge my fuckin' battery!\"");
                exit(0);
                    */
            }
        }
#endif
    }
#if verbosity > 1
    printf("returning %d from getbattery()\n", capacity);
#endif
    return capacity;
}

static char *net(void) {
#if verbosity > 1
    printf("now entering net()\n");
#endif
    FILE *fp;
    /* fp = popen("ping -c 1 -W 1 google.com > /dev/null 2>&1 && \
               echo 'NET: ON' || echo 'NET OFF'", "r"); */
    fp = popen("ping -c 1 -W 1 google.com > /dev/null 2>&1 && \
               echo 'ON' || echo 'OFF'", "r");
    if (fp == NULL) {
        return "err";
    } 
    char *output;
    output = malloc(4);
    fgets(output, 4, fp);
    pclose(fp);

    // the last character might be an unwanted newline
    if (output[strlen(output)-1] == '\n') {
        output[strlen(output)-1] = '\0';
    }
#if verbosity > 1
    printf("returning %s from net()\n", output);
#endif
    return output;
}

double * getavgs(void) {
    // THIS is PRETTy DUMB! 
    // you could just read from /proc/loadavg instead...
    // must return exactly 3 doubles 
    double * avgs = malloc(3 * sizeof(double));
    int num_avgs = getloadavg(avgs, 3);
    if (num_avgs < 3) {
        if (num_avgs == -1) {
            fprintf(stderr, "num_avgs is -1");
            num_avgs = 0;
        } 
        for (int i = num_avgs; i < 3; i++) {
            avgs[i] = 9.9;
        }
    }
    return avgs;
}

/*
char * getavgs(void) {
    double avgs[3];
    if (getloadavg(avgs, 3) < 0) {
        perror("getloadavg broke");
        // return "oops";
        exit(1);
    }
    return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}
*/

int main(void) {
    daemon(0, 0);
    // int num_avgs;
    // double avgs[3];
    static Display *dpy;
    char *status;

    double * avgs;
    // printf("THE NUMBER: %f\n", avgs[0]);

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display.\n");
        return EXIT_FAILURE;
    }
    if ((status = malloc(80)) == NULL) {
        fprintf(stderr, "status = malloc(80) failed.\n");
        return EXIT_FAILURE;
    }
    
    for ( ; ; sleep(59)) {
        avgs = getavgs();

        snprintf(status, 90, OUTFORMAT, 
                 // getfiledata("/tmp/dwm-status.mail"), 
                 avgs[0], avgs[1], avgs[2],
                 getbattery(), 
                 getfiledata(MAILFILE), 
                 getfiledata(PKGFILE), 
                 // getfiledata("/tmp/dwm-status.packages"),
                 net(), getdatetime());
#if verbosity > 0
        puts(status);
#endif
        setstatus(status, dpy);
    }

    free(status);
    XCloseDisplay(dpy);
    return 0;
}
