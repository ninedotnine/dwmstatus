char *getdatetime(void); 
int getfiledata(const char *filename); 
char * getTemperature(void);
char * getBattery(void); 
char *net(void); 
double * getAvgs(void); 
void usage(FILE * stream, int exit_code); 
void setStatus(Display *dpy);
char * buildStatus(void);
