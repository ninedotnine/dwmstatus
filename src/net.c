#include "net.h"

#include <errno.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



char net_buf[MAX_NET_MSG_LEN];
bool noNetwork = false;

void net(void) {
    if (noNetwork) {
        int success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s?%s", COLO_DEEPGREEN, COLO_RESET);
        if (success > MAX_NET_MSG_LEN) {
            fputs("net output truncated\n", stderr);
        } else if (success < 1) {
            fputs("error, problem with snprintf\n", stderr);
            exit(15);
        }
        return;
    }
    struct addrinfo * info = NULL;
    int error = getaddrinfo("google.com", "80", NULL, &info);
    if (error != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(error));
        int success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s", COLO_RED "NET" COLO_RESET);
        if (success > MAX_NET_MSG_LEN) {
            fputs("net output truncated\n", stderr);
        } else if (success < 1) {
            fputs("error, problem with snprintf\n", stderr);
            exit(12);
        }
    } else if (info == NULL) {
        fprintf(stderr, "net error: error is 0 but info is null\n");
        int success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s", COLO_RED "NET" COLO_RESET);
        if (success > MAX_NET_MSG_LEN) {
            fputs("net output truncated\n", stderr);
        } else if (success < 1) {
            fputs("error, problem with snprintf\n", stderr);
            exit(13);
        }
    } else {
        int sockfd = socket(info->ai_family,
                            info->ai_socktype,
                            info->ai_protocol);
        int success; // check the return value of snprintf
        if (connect(sockfd, info->ai_addr, info->ai_addrlen) == -1) {
            int errnum = errno;
            fprintf(stderr, "errno is: %i\n", errnum);
            success = snprintf(net_buf, MAX_NET_MSG_LEN, "%sNET%s", COLO_YELLOW, COLO_RESET);
        } else {
            success = snprintf(net_buf, MAX_NET_MSG_LEN, "%s5x5%s", COLO_DEEPGREEN, COLO_RESET);
        }
        close(sockfd);
        if (success > MAX_NET_MSG_LEN) {
            fputs("net output truncated\n", stderr);
        } else if (success < 1) {
            fputs("error, problem with snprintf\n", stderr);
            exit(14);
        }
    }
    freeaddrinfo(info);
}
