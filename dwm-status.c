/* 
 * 9.9
 * dan: compile with: gcc -Wall -pedantic -lX11 -std=c99 dwm-status.c
 * or -D experimental_alarm to see what happens (nothing useful)
 * last updated aug 24 2014
 * also, replace getAvgs() with a read from /proc/loadavg
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

// ideally this would only be defined if i knew zenity was installed
// but fuck that, right???? who even needs makefiles
#define zenity

// NDEBUG turns off all assert() calls
// #define NDEBUG

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
#include <netdb.h>
// #include <sys/types.h>
// #include <sys/socket.h>

#include "dwm-status-defs.h"

const char * program_name;
bool verboseMode;


void setstatus(const char *str, Display *dpy) {
    assert (dpy != NULL);
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char *getdatetime(void) {
    char *buf;
    time_t result;
    struct tm *resulttm;

    if ((buf = malloc(65 * sizeof(char))) == NULL) {
        fprintf(stderr, "Cannot allocate memory for buf.\n");
        return "time ???";
    }
    result = time(NULL);
    resulttm = localtime(&result);
    if (resulttm == NULL) {
        fprintf(stderr, "Error getting localtime.\n");
        return "time ???";
    }
    if (! strftime(buf, sizeof(char)*65-1, TIMESTRING, resulttm)) {
        fprintf(stderr, "strftime is 0.\n");
        return "time ???";
    }
    return buf;
}

int getfiledata(const char *filename) {
    // this function parses an int from filename 
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
    return result;
}

char * getTemperature(void) {
    float temper = (float) getfiledata(TEMPERATURE) / 1000.0;
    char * result; // will be returned
    int code; // for colouring
    if (temper > 75) {
        code = 2; // yellow
    } else if (temper > 85) {
        code = 1; // red
    } else if (temper < 60) {
        code = 6; // blue 
    } else {
        code = -1; // empty string 
    }
    asprintf(&result, "%s%02.1f°C%s", getColour(code), temper, getColour(0));
    return result;
}


char * getBattery(void) {
    int capacity;
    bool chargin = false;
//     capacity = getfiledata("/sys/class/power_supply/BAT0/capacity");
    capacity = getfiledata(BATT_CAPACITY);

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

    if (capacity < 95) {
        FILE *fd;
        fd = fopen(BATT_STATUS, "r");
        if (fd == NULL) {
            fprintf(stderr, "Error opening BATT_STATUS.\n");
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
            fprintf(stderr, "low battery warning\n");
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
    int code;
    if (chargin) {
        code = 4; // magenta
    } else if (capacity > 70) {
        code = 3; // deep green
    } else if (capacity > 30) {
        code = 6; // blue
    } else if (capacity > WARN_LOW_BATT) {
        code = 2; // yellow
    } else {
        code = 1; // red
    }


    if (asprintf(&result, "%s%i%%%s", getColour(code), capacity, getColour(0)) 
            == -1) {
        fprintf(stderr, "whoooaaa, test in getBattery was -1");
        exit(EXIT_FAILURE);
    }
    return result;
}

void setup_addrinfo(struct addrinfo ** info) {
    printf("setup_addrinfo\n");
    int error = getaddrinfo("google.com", "80", NULL, info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
    }   
    printf("end setup_addrinfo\n");
    assert (info != NULL && *info != NULL);
}

char * net(void) {
//     printf("start newnet()\n");

    struct addrinfo * info = NULL;
//     setup_addrinfo(&info);
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
        return "error";
    }   
    assert (info != NULL);

//     printf("making socket\n");
    int sockfd = socket(info->ai_family, info->ai_socktype,info->ai_protocol);
//     printf("connecting \n");
    char * result; // will be returned
    int success; // check the return value of asprintf
    if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
        success = asprintf(&result, "%sNET%s", getColour(1), getColour(0));
    } else {
        success = asprintf(&result, "%sOK%s", getColour(3), getColour(0));
    }
    if (success == -1) {
        fprintf(stderr, "error, unable to malloc() in asprintf()");
        return "error";
    }
    return result;
}

double * getAvgs(void) {
    // THIS is PRETTy DUMB! 
    // you could just read from /proc/loadavg instead...
    // must return exactly 3 doubles 
    double * avgs = malloc(3 * sizeof(double));
    int num_avgs = getloadavg(avgs, 3);
    assert (num_avgs == 3);
//     assert (getloadavg(avgs, 3) == 3);
    /*
    if (num_avgs < 3) {
        if (num_avgs == -1) {
            fprintf(stderr, "num_avgs is -1");
            num_avgs = 0;
        } 
        for (int i = num_avgs; i < 3; i++) {
            avgs[i] = 9.9;
        }
    }
    */
    return avgs;
}

/*
char * getAvgs(void) {
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
    fprintf(stream, " -v --verbose      verbose mode\n");
    fprintf(stream, " -d --daemon       run in background\n");
    fprintf(stream, " -r --report       report status immediately (default)\n");
    exit(exit_code);
}

char * getColour(int code) {
    // one way to display all available colours: weechat --colors
    // positive codes return various colours
    // code 0 returns a reset string
    // code -1 returns empty string
    switch (code) {
        case 0: 
            return "\x1b[0m"; // reset 
        case 1:
            return "\x1b[38;5;196m"; // red
        case 2:
            return "\x1b[38;5;190m"; // yellow
        case 3:
            return "\x1b[38;5;34m"; // deep green
        case 4:
            return "\x1b[38;5;199m"; // magenta
        case 5:
            return "\x1b[38;5;46m"; // bright green
        case 6:
            return "\x1b[38;5;21m"; // blue
        default:
            return "";
    }
}

int main(int argc, char * argv[]) {
    program_name = argv[0];

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
//     bool verboseMode = false;
    verboseMode = false;

    do {
        nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);
        switch (nextOption) {
            case 'h':
                usage(stdout, 0);
            case 'v':
//                 printf("goin into verbose mode\n");
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

    // updateStatus once first to initialize the static variables sent and recv
    updateStatus(dpy); // i know, it's wasteful

    if (daemonMode) {
        daemon(0,0);
        for (; ; sleep(SLEEP_INTERVAL)) {
            updateStatus(dpy);
        }
    } 

    updateStatus(dpy);
    /*
    if (verboseMode) { // this is currently the only place verboseMode is used
        printf("now quittin!\n"); // LOL
    }
    */
    return 0;
}

void updateStatus(Display *dpy) {
	static unsigned long long int recv, sent;
    static char * status;
    double * avgs = getAvgs();
    // calling parse_netdev() first will initialize recv and sent to
    // useful values, but if i do this here, parse_netdev() will be called
    // every time updateStatus() is called, resulting in bad values
// 	parse_netdev(&recv, &sent);

    // thank you, _GNU_SOURCE, for asprintf
    // asprintf returns -1 on error, we check for that 
    if (asprintf(&status, OUTFORMAT,
                 avgs[0], avgs[1], avgs[2],
                 get_netusage(&recv, &sent),
                 getBattery(),
                 getTemperature(),
                 getfiledata(MAILFILE), 
                 net(), getdatetime()) == -1) {
        fprintf(stderr, "error, unable to malloc() in asprintf()");
        exit(1);
    }

    if (verboseMode) {
        printf("%s\n", status);
    }

    setstatus(status, dpy);
    free(status);
}


/* --------------------------------------------------  
   net stuff begins here, with thanks to anonymous suckless contributor
   */

bool parse_netdev(unsigned long long int *receivedabs, 
                  unsigned long long int *sentabs) {
	static const int bufsize = 255;
	char buf[bufsize];
	char *datastart;
	bool success; // will be returned
	FILE *devfd;
	unsigned long long int receivedacc, sentacc;

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

char * calculate_speed(unsigned long long int newval, 
                       unsigned long long int oldval) {
    char * speedstr; // will be returned
	double speed;
    char unit;

	speed = (newval - oldval) / 1024.0;
	if (speed > 1024.0) {
	    speed /= 1024.0;
        unit = 'M';
	} else {
        unit = 'k';
	}

    // pad the beginning of the string so it uses the same space for all sizes
    if (asprintf(&speedstr, "%6.2f%c", speed, unit) == -1) {
        return "??";
    }
    return speedstr;
}

char * get_netusage(unsigned long long int *recv, 
                    unsigned long long int *sent) {
    // this function has the side effect of updating the values of recv and sent
	unsigned long long int newrecv, newsent;
	newrecv = newsent = 0;

	bool retval = parse_netdev(&newrecv, &newsent);
	if (! retval) {
	    fprintf(stderr, "Error when parsing /proc/net/dev file.\n");
	    return "V: ?? Λ: ??";
	}

	char * downspeedstr = calculate_speed(newrecv, *recv);
	char * upspeedstr = calculate_speed(newsent, *sent);

	char * netusage; // will be returned
	if (asprintf(&netusage, "V:%s Λ:%s", downspeedstr, upspeedstr) == -1) {
	    fprintf(stderr, "Error when parsing /proc/net/dev file.\n");
	    return "V: ?? Λ: ??";
    }

	*recv = newrecv;
	*sent = newsent;
	return netusage;
}
