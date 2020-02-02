#pragma once

#include <stdio.h>

void getdatetime(char ** input);
int getfiledata(const char * filename);
void getTemperature(char ** result);
void getBattery(char ** batt);
void net(char ** netOK);
void handle_mpd_error(struct mpd_connection *c);
void getAvgs(double (* avgs)[3]);
void usage(FILE * stream, int exit_code);
void setStatus(void);
char * buildStatus(void);
void * mpd_idler(__attribute__((unused)) void * arg);
