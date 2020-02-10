#pragma once

#include "config.h"

#include <X11/Xlib.h>

extern Display *dpy;

void set_status(const char * net_buf);
void set_status_to(const char * string);
void build_status(const char * net_str, char * buffer);
