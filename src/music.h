#pragma once

#include "dwmstatus-defs.h"

#include <mpd/client.h>

struct mpd_connection * establish_mpd_conn(void);

void get_now_playing(char buffer[MPD_STR_LEN]);
void * mpd_idler(void * arg);
