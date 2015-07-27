char *getdatetime(void); 
int getfiledata(const char *filename); 
char * getTemperature(void);
char * getBattery(void); 
char *net(void); 
void getAvgs(double (* avgs2)[3]); 
void usage(FILE * stream, int exit_code); 
void setStatus(Display *dpy);
char * buildStatus(void);
