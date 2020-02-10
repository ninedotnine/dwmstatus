#pragma once

#include "config.h"

void open_x11(void);
void set_status(const char * net_buf);
void set_status_to(const char * string);
void build_status(const char * net_str, char * buffer);
