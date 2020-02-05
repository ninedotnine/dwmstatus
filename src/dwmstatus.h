#pragma once

#include <stdio.h>

void get_time(char buffer[static TIME_STR_LEN]);
int read_int_from_file(const char * filename);
void getTemperature(char ** result);
void get_batt(char buffer[static BATT_STR_LEN]);
void handle_mpd_error(struct mpd_connection *c);
void getAvgs(double (* avgs)[3]);
void usage(FILE * stream, int exit_code);
void setStatus(char * net_buf);
void set_status_to(const char * string);
char * buildStatus(char * net_buf);
void * mpd_idler(__attribute__((unused)) void * arg);
