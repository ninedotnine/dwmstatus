/* 
 * 9.9
 * dan: compile with: gcc -Wall -pedantic -lX11 -std=c99 dwm-status.c
 * pass the argument -Dverbosity=2 when compiling for output
 * or -D experimental_alarm to see what happens (nothing useful)
 * last updated aug 04 2014
 * also, replace getavgs() with a read from /proc/loadavg
 * make WARN_LOW_BATT_TEXT do something, the ""s make it tricky
 * add cmdline args to either print immediately or daemon
 *     make verboseMode do something or get rid of it
 * colours!!!
 */

// this makes gcc not complain about implicit declarations of popen and pclose
// when using -pedantic and -std=c99. i guess that's good?
// also provides getloadavg and asprintf
// see: man 7 feature_test_macros
#define _GNU_SOURCE

/* SETTINGS */

// the format for the date/time
// const char timestring[] = "[%a %b %d] %H:%M";
#define TIMESTRING "[%a %b %d] %H:%M"
// the format for everything
// const char outformat[] = "[batt: %d%%] [mail %d] [pkg %d] [net %s] %s";
// #define OUTFORMAT "[%.2f %.2f %.2f] [batt: %d%%] [mail %d] [pkg %d] [net %s] %s"
#define OUTFORMAT "[%.2f %.2f %.2f] [%s] [%s] [mail %d] [net %s] %s"
// #define OUTFORMAT "[%.2f %.2f %.2f] [%s] [batt: %d%%] [mail %d] [net %s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
#define WARN_LOW_BATT_TEXT "you're a fat slut"
// files -- populated by cron
#define MAILFILE "/tmp/dwm-status.mail"
#define PKGFILE "/tmp/dwm-status.packages"
// #define FBCMDFILE "/tmp/dwm-status.fbcmd"

// ideally this would only be defined if i knew zenity was installed
// but fuck that, right???? who even needs makefiles
#define zenity

// NDEBUG turns off all assert() calls
// #define NDEBUG

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
#include <getopt.h>
#include <stdbool.h>
#include "dwm-status.h"
// #include <netdb.h>

const char * program_name;

void setstatus(const char *str, Display *dpy) {
    assert(dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char *getdatetime(void) {
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

int getfiledata(const char *filename) {
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

char * getbattery(void) {
#if verbosity > 1
    printf("entering getbattery()\n");
#endif
    int capacity;
    bool chargin = false;
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

//     if (capacity <= WARN_LOW_BATT) {
//     if (capacity < 95) {
    if (capacity < 100) {
        FILE *fd;
        fd = fopen("/sys/class/power_supply/BAT0/status", "r");
        if (fd == NULL) {
            fprintf(stderr, "Error opening BAT0/status.\n");
            return "??%";
        }

        char * status;
        status = malloc(15);
//         fscanf(fd, "%s", status);
        fgets(status, 15, fd);
        fclose(fd);

        chargin = 0 != strncmp(status, "Discharging", 11);
        printf("chargin: %s\n", (chargin) ? "true" : "false");
//         if ( ! strncmp(status, "Discharging", 11) ) {
//         if ( ! chargin) {
        if (! chargin && capacity <= WARN_LOW_BATT) {
            fprintf(stderr, "low battery warning\n");
#ifdef zenity
            // display a warning on low battery and not plugged in. 
            // depends on zenity
            if (! fork()) { 
                char * const args[] = {"zenity", "--warning", "--text=you're a fat slut", NULL};
//                 execv("zenity", {"--warning", "--text=what", NULL});
                execv("/usr/bin/zenity", args);
                    /*
                system("zenity --warning \
                        --text=\"charge my fuckin' battery!\"");
                exit(0);
                    */
            }
#endif
        }
    }
#if verbosity > 1
    printf("returning %d from getbattery()\n", capacity);
#endif

    // colourize the result
    char * result;
    int code;
//     if (capacity > WARN_LOW_BATT + 20) {
    if (capacity > 95) {
        code = 3; // deep green
    } else if (chargin) {
        code = 4; // magenta
    } else if (capacity > WARN_LOW_BATT && capacity < WARN_LOW_BATT + 20) {
        code = 2; // yellow
    } else if (capacity < WARN_LOW_BATT) {
        code = 1; // red
    } else {
        code = 0;
    }

    int test = asprintf(&result, "%s%i%%%s", getColour(code), capacity, getColour(0));
    if (test == -1) {
        fprintf(stderr, "whoooaaa, test in getbattery was -1");
        exit(EXIT_FAILURE);
    }
    return result;

}

char *net(void) {
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

    char * result = malloc(40);
    snprintf(result, 15, "%s", getColour(3));
    
    strcat(result, output);

    // the last character might be an unwanted newline
    /*
    if (output[strlen(output)-1] == '\n') {
        output[strlen(output)-1] = '\0';
    }
    */
    if (result[strlen(result)-1] == '\n') {
        result[strlen(result)-1] = '\0';
    }
    strcat(result, "\x1b[0m");
#if verbosity > 1
    printf("returning %s from net()\n", output);
#endif
//     return output;
    return result;
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

void usage(FILE * stream, int exit_code) {
    // prints what's up to stream, then quits with status exit_code
    fprintf(stream, "usage: %s [args]\n", program_name);
    fprintf(stream, " -h --help         display this message\n");
    fprintf(stream, " -v --verbose      verbose mode, does nothing LOL\n");
    fprintf(stream, " -d --daemon       run in background\n");
    fprintf(stream, " -r --report       report status immediately (default)\n");
    exit(exit_code);
}



 // obviously do this right
// char * getColour(void) {
char * getColour(int code) {
    char * colour;
    if (code == 0) {
        colour = "\x1b[0m"; // reset 
    } else if (code == 1) { 
        colour = "\x1b[38;5;196m"; // reset
    } else if (code == 2) {
        colour = "\x1b[38;5;190m"; // yellow
    } else if (code == 3) {
        colour = "\x1b[38;5;34m"; // deep green
    } else if (code == 4) {
        colour = "\x1b[38;5;199m"; // magenta
    } else {
        colour = "\x1b[38;5;46m"; // bright green
    }

    /*
    char * colour = malloc(100);
    snprintf(colour, 100, "\x1b[38;5;196m"); // red
    */
    return colour;
}

int main(int argc, char * argv[]) {

    program_name = argv[0];
    
    bool verboseMode = false;
    /* list available args */
    const char * const shortOptions = "hvdr";
    const struct option longOptions[] = {
        {"help", 0, NULL, 'h'},
        // if verboseMode was an int, you could do this:
        // this means if --verbose is in the args, set verboseMode to 1
//         {"verbose", 0, &verboseMode, 1}, 
        {"verbose", 0, NULL, 'v'}, 
        {"daemon", 0, NULL, 'd'},
        {"report", 0, NULL, 'r'},
    };

    /* parse args */
    int nextOption;
    bool daemonMode = false;

    do {
        nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);
        switch (nextOption) {
            case 'h':
                usage(stdout, 0);
            case 'v':
                printf("goin into verbose mode LOL NOT REALLY\n");
                verboseMode = true;
                break;
            case 'd':
//                 printf("goin into daemon mode\n");
                daemonMode = true;
                break;
            case 'r':
//                 printf("goin into default mode\n");
                daemonMode = false;
                break;
            case '?':
                usage(stderr, 1);
            case (-1): 
                break;
            default:
                // when does this happen? 
                printf("what? exiting\n");
                exit(100);
        }
    } while (nextOption != -1);

    static Display *dpy;
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Cannot open display.\n");
        return EXIT_FAILURE;
    }

    if (daemonMode) {
        return daemonize(dpy);
    } 

    updateStatus(dpy);
    if (verboseMode) { // this is currently the only place verboseMode is used
        printf("now quittin!\n"); // LOL
    }
    return 0;
}

void updateStatus(Display *dpy) {
//     printf("%s\n", getColour() + 1);
	static unsigned long long int recv, sent;
    static char * status;
    double * avgs = getavgs();
    // calling parse_netdev() first will initialize recv and sent to
    // useful values, but if i do this here, parse_netdev() will be called
    // every time updateStatus() is called, resulting in bad values
// 	parse_netdev(&recv, &sent);

    // thank you, _GNU_SOURCE, for asprintf
    // asprintf returns -1 on error, we check for that 

    if (asprintf(&status, OUTFORMAT,
             avgs[0], avgs[1], avgs[2],
             get_netusage(&recv, &sent),
             getbattery(),
             getfiledata(MAILFILE), 
//                  getfiledata(PKGFILE), 
             net(), getdatetime()) == -1) {
        fprintf(stderr, "error, unable to malloc() in asprintf()");
        exit(1);
    }

    setstatus(status, dpy);
    free(status);
    // obviously do this right
//     setstatus(strcat(getColour(), status), dpy);

    /*
    if (verboseMode) {
        printf("%s\n", status);
    }
    */
}


int daemonize(Display * dpy) {
//     printf("running daemon HEHE\n");
//     daemon(0, 0);
  
// 	static unsigned long long int recv, sent;
// 	parse_netdev(&recv, &sent);
    for (; ; sleep(1)) {
    /*
    for (int x = 0; ; sleep(4)) {
        printf("iteration %i!\n", x);
        x++;
        */
        updateStatus(dpy);
    }
    return 0;
}

/* --------------------------------------------------  
   net stuff begins here, with thanks to anonymous suckless contributor
   */

bool parse_netdev(unsigned long long int *receivedabs, 
                  unsigned long long int *sentabs) {
	char buf[255];
	char *datastart;
	static int bufsize;
	bool success;
	FILE *devfd;
	unsigned long long int receivedacc, sentacc;

	bufsize = 255;
	devfd = fopen("/proc/net/dev", "r");
	success = false;

	// Ignore the first two lines of the file
	fgets(buf, bufsize, devfd);
	fgets(buf, bufsize, devfd);

	while (fgets(buf, bufsize, devfd)) {
	    if ((datastart = strstr(buf, "lo:")) == NULL) {
            datastart = strstr(buf, ":");

            // With thanks to the conky project at http://conky.sourceforge.net/
            sscanf(datastart + 1, "%llu  %*d     %*d  %*d  %*d  %*d   %*d        %*d       %llu",\
                   &receivedacc, &sentacc);
            *receivedabs += receivedacc;
            *sentabs += sentacc;
            success = true;
	    }
	}

	fclose(devfd);
	return success;
}

void calculate_speed(char *speedstr, unsigned long long int newval, 
                     unsigned long long int oldval) {
	double speed;
    char unit;
	speed = (newval - oldval) / 1024.0;
	if (speed > 1024.0) {
	    speed /= 1024.0;
        unit = 'm';
// 	    sprintf(speedstr, "%.3f MB/s", speed);
// 	    sprintf(speedstr, "%.2fm", speed);
	} else {
        unit = 'k';
// 	    sprintf(speedstr, "%.2f KB/s", speed);
// 	    sprintf(speedstr, "%.2fk", speed);
	}
    // pad the beginning of the string so it uses the same space for all sizes
    char * padding;
    if (speed < 10) {
        padding = "  ";
    } else if (speed < 100) {
        padding = " ";
    } else {
        padding = "";
    }
    sprintf(speedstr, "%s%.2f%c", padding, speed, unit);

}

char * get_netusage(unsigned long long int *recv, 
                    unsigned long long int *sent) {
    // this function has the side effect of updating the values of recv and sent
	unsigned long long int newrecv, newsent;
	newrecv = newsent = 0;

	bool retval = parse_netdev(&newrecv, &newsent);
	if (! retval) {
	    fprintf(stdout, "Error when parsing /proc/net/dev file.\n");
	    exit(1);
	}

	char downspeedstr[15], upspeedstr[15];
	calculate_speed(downspeedstr, newrecv, *recv);
	calculate_speed(upspeedstr, newsent, *sent);

	static char retstr[42];
// 	sprintf(retstr, "down: %s up: %s", downspeedstr, upspeedstr);
	sprintf(retstr, "V:%s Λ:%s", downspeedstr, upspeedstr);

	*recv = newrecv;
	*sent = newsent;
	return retstr;
}
