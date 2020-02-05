#pragma once

#include <stdio.h>

void getdatetime(char buffer[static 32]);
int getfiledata(const char * filename);
void getTemperature(char ** result);
void getBattery(char ** batt);
void handle_mpd_error(struct mpd_connection *c);
void getAvgs(double (* avgs)[3]);
void usage(FILE * stream, int exit_code);
void setStatus(char * net_buf);
void set_status_to(const char * string);
char * buildStatus(char * net_buf);
void * mpd_idler(__attribute__((unused)) void * arg);
