static void setstatus(const char *str, Display *dpy);
static char *getdatetime(void); 
static int getfiledata(const char *filename); 
static int getbattery(void); 
static char *net(void); 
double * getavgs(void); 
void usage(FILE * stream, int exit_code); 
int daemonize(Display *dpy);
void updateStatus(Display * dpy);

// net stuff begins here

bool parse_netdev(unsigned long long int *receivedabs, 
                  unsigned long long int *sentabs);
void calculate_speed(char *speedstr, unsigned long long int newval, 
                     unsigned long long int oldval);
char * get_netusage(unsigned long long int *rec, unsigned long long int *sent);


