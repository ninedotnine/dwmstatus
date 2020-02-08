#pragma once

#include <stdio.h>

static void get_time(char buffer[static TIME_STR_LEN]);
static int read_int_from_file(const char * filename);
static void get_temperature(char * buffer);
static void get_batt(char buffer[static BATT_STR_LEN]);
static void get_avgs(double avgs[static 3]);
static void usage(FILE * stream, int exit_code);
static void setStatus(char * net_buf);
static void set_status_to(const char * string);
static void build_status(const char * net_str, char * buffer);
static void * mpd_idler(__attribute__((unused)) void * arg);
