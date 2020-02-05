#pragma once

#include "dwmstatus-defs.h"

#include <stdbool.h>
#include <pthread.h>

extern pthread_mutex_t net_buf_mutex;

void update_net_buffer(char * net_buf);
void * network_updater(__attribute__((unused)) void * arg);
