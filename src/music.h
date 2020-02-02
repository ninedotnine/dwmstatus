#pragma once

#include <pthread.h>
#include <mpd/client.h>

extern pthread_mutex_t mutex;

void handle_mpd_error(struct mpd_connection *c);

struct mpd_connection * establish_mpd_conn(void);

void getNowPlaying(char * (* const string));
