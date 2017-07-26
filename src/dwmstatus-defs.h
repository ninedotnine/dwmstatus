/* SETTINGS */

// the format for the date/time
#define TIMESTRING "[%a %b %d] %H:%M"
// the format for everything
#define OUTFORMAT "%s[%.2f %.2f %.2f] [%s] [%s] [%s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
// what the battery warning says
#define WARN_LOW_BATT_TEXT "you know if there's one thing i really respect, \nit's plus-size models who challenge the idea of what makes a woman beautiful"

// 10 seconds between iterations
#define SLEEP_INTERVAL 20

// locations of files
#define TEMPERATURE   "/sys/class/thermal/thermal_zone0/temp"
#define BATT_CAPACITY "/sys/class/power_supply/BAT0/capacity"
#define BATT_STATUS   "/sys/class/power_supply/BAT0/status"

#define COLO_RESET "\x1b[0m" // reset
#define COLO_RED "\x1b[38;5;196m" // red
#define COLO_YELLOW "\x1b[38;5;190m" // yellow
#define COLO_DEEPGREEN "\x1b[38;5;34m" // deep green
#define COLO_MAGENTA "\x1b[38;5;199m" // magenta
#define COLO_CYAN "\x1b[38;5;45m" // bright blue
#define COLO_BLUE "\x1b[38;5;32m" // blue
