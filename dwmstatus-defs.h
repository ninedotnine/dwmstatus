/* SETTINGS */

// the format for the date/time
// const char TIMESTRING[] = "[%a %b %d] %H:%M";
#define TIMESTRING "[%a %b %d] %H:%M"
// the format for everything
// const char OUTFORMAT[] "[%.2f %.2f %.2f] [%s] [%s] [%s] [mail %d] [%s] %s"
// #define OUTFORMAT "[%.2f %.2f %.2f] [%s] [%s] [mail %d] [%s] %s"
#define OUTFORMAT "[%.2f %.2f %.2f] [%s] [%s] [%s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
// this is not currently implemented at all...
#define WARN_LOW_BATT_TEXT "FIXME"

#define SLEEP_INTERVAL 8

// locations of files
// #define TEMPERATURE   "/sys/class/hwmon/hwmon0/device/temp1_input"
#define TEMPERATURE   "/sys/class/thermal/thermal_zone0/temp"
#define BATT_CAPACITY "/sys/class/power_supply/BAT0/capacity"
#define BATT_STATUS   "/sys/class/power_supply/BAT0/status"

// files -- populated by cron
// #define MAILFILE "/tmp/dwm-status.mail"
// #define PKGFILE "/tmp/dwm-status.packages"
// #define FBCMDFILE "/tmp/dwm-status.fbcmd"

#define COLO_RESET "\x1b[0m" // reset
#define COLO_RED "\x1b[38;5;196m" // red
#define COLO_YELLOW "\x1b[38;5;190m" // yellow
#define COLO_DEEPGREEN "\x1b[38;5;34m" // deep green
#define COLO_MAGENTA "\x1b[38;5;199m" // magenta
#define COLO_BRIGHTGREEN "\x1b[38;5;46m" // bright green
#define COLO_CYAN "\x1b[38;5;45m" // bright blue
#define COLO_BLUE "\x1b[38;5;21m" // blue
