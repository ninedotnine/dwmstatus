#pragma once

#include "dwmstatus-defs.h"

#include <stdbool.h>
#include <pthread.h>

extern pthread_mutex_t net_buf_mutex;
extern char net_buf[MAX_NET_MSG_LEN];

void net(void);
void * network_updater(__attribute__((unused)) void * arg);
