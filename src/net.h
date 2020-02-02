#pragma once

#define MAX_NET_MSG_LEN 99

#include "dwmstatus-defs.h"

#include <stdbool.h>

extern char net_buf[MAX_NET_MSG_LEN];
extern bool noNetwork;

void net();
