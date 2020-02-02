#include "net.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


pthread_mutex_t net_buf_mutex = PTHREAD_MUTEX_INITIALIZER;
char net_buf[MAX_NET_MSG_LEN];

void update_net_buffer(void) {
    struct addrinfo * info = NULL;
    int snprintf_ret = 0;
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
        snprintf_ret = snprintf(net_buf, MAX_NET_MSG_LEN, "%s", COLO_RED "NET" COLO_RESET);
    } else if (info == NULL) {
        fprintf(stderr, "net error: error is 0 but info is null\n");
        snprintf_ret = snprintf(net_buf, MAX_NET_MSG_LEN, "%s", COLO_RED "NET" COLO_RESET);
    } else {
        int sockfd = socket(info->ai_family,
                            info->ai_socktype,
                            info->ai_protocol);
        if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
            int errnum = errno;
            fprintf(stderr, "errno is: %i\n", errnum);
            snprintf_ret = snprintf(net_buf, MAX_NET_MSG_LEN, "%sNET%s", COLO_YELLOW, COLO_RESET);
        } else {
            snprintf_ret = snprintf(net_buf, MAX_NET_MSG_LEN, "%s5x5%s", COLO_DEEPGREEN, COLO_RESET);
        }
        close(sockfd);
    }
    freeaddrinfo(info);

    if (snprintf_ret > MAX_NET_MSG_LEN) {
        fputs("net output truncated\n", stderr);
    } else if (snprintf_ret < 1) {
        fputs("error, problem with snprintf\n", stderr);
        exit(12);
    }
}

void * network_updater(__attribute__((unused)) void * arg) {
    int success = pthread_detach(pthread_self());
    assert (success == 0);
    while (true) {
        success = pthread_mutex_lock(&net_buf_mutex);
        assert(success == 0);
        update_net_buffer();
        success = pthread_mutex_unlock(&net_buf_mutex);
        assert(success == 0);
        sleep(SLEEP_INTERVAL);
    }
}
