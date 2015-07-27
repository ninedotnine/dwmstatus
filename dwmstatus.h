char *getdatetime(void);
int getfiledata(const char *filename);
void getTemperature(char * (* const result));
void getBattery(char * (* const batt));
void net(char * (* const netOK));
void getAvgs(double (* avgs2)[3]);
void usage(FILE * stream, int exit_code);
void setStatus(Display *dpy);
char * buildStatus(void);
