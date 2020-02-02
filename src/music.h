#pragma once

#include <mpd/client.h>

struct mpd_connection * establish_mpd_conn(void);

void getNowPlaying(char * (* const string));
