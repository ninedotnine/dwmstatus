#include "net.h"

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t net_buf_mutex = PTHREAD_MUTEX_INITIALIZER;

// this function locks the net_buf_mutex before writing to net_buf
static void write_to_net_buf(char * net_buf, const char * msg) {
    int lock_ret = pthread_mutex_lock(&net_buf_mutex);
    assert(lock_ret == 0);

    int length = snprintf(net_buf, MAX_NET_MSG_LEN, "%s", msg);

    if (length > MAX_NET_MSG_LEN) {
        fputs("net output truncated\n", stderr);
    } else if (length < 1) {
        fputs("error, problem with snprintf\n", stderr);
        exit(14);
    }

    lock_ret = pthread_mutex_unlock(&net_buf_mutex);
    assert(lock_ret == 0);
}

void update_net_buffer(char * net_buf) {
    struct addrinfo * info = NULL;
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
        write_to_net_buf(net_buf, COLO_RED "NET" COLO_RESET);
    } else if (info == NULL) {
        fprintf(stderr, "net error: error is 0 but info is null\n");
        write_to_net_buf(net_buf, COLO_RED "NET" COLO_RESET);
    } else {
        int sockfd = socket(info->ai_family,
                            info->ai_socktype,
                            info->ai_protocol);
        if (sockfd < 0) {
            fprintf(stderr, "net error: sockfd < 0; %s\n", strerror(errno));
            write_to_net_buf(net_buf, COLO_RED "NET?" COLO_RESET);
        } else {
            error = connect(sockfd, info->ai_addr, info->ai_addrlen);
            if (error == -1) {
                fprintf(stderr, "errno is: %i\n", errno);
                write_to_net_buf(net_buf, COLO_YELLOW "NET" COLO_RESET);
            } else {
                assert (error == 0);
                write_to_net_buf(net_buf, COLO_DEEPGREEN "5x5" COLO_RESET);
            }
        }
        close(sockfd);
    }
    freeaddrinfo(info);
}

void * network_updater(void * buffer) {
    char * net_buf = buffer;
    int detach_ret = pthread_detach(pthread_self());
    assert (detach_ret == 0);
    while (true) {
        update_net_buffer(net_buf);
        sleep(SLEEP_INTERVAL);
    }
}
