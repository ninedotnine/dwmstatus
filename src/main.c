#include "music.h"
#include "net.h"
#include "utils.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>

static const char * program_name;

static void handle_signal(__attribute__((unused)) int sig) {
    // do nothing...
}

static void usage(FILE * stream, int exit_code) {
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
    int length = snprintf(net_buf, MAX_NET_MSG_LEN, "%s?%s", COLO_DEEPGREEN, COLO_RESET);
    if (length > MAX_NET_MSG_LEN) {
        fputs("the net buffer is too small.\n", stderr);
    } else if (length < 1) {
        fputs("error, problem with snprintf\n", stderr);
        exit(16);
    }

    if (daemon_mode || update_mode) {
        open_x11();
    }

    if (daemon_mode) {
        if (! run_in_foreground) {
            int daemon_ret;
            if (be_quiet) {
                daemon_ret = daemon(0, 0);
            } else {
                daemon_ret = daemon(0, 1);
            }
            if (daemon_ret != 0) {
                fprintf(stderr, "failure to daemonize, %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            assert (daemon_ret == 0);
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
        int sigaction_ret = sigaction(SIGUSR1, &sa, NULL);
        assert (sigaction_ret == 0);

        while (true) {
            set_status(net_buf);
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
