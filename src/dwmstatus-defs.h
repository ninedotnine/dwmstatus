/* SETTINGS */
#define VERSION "0.5.1"

// the format for the date/time
#define TIMESTRING "%a %d %b  %H:%M"
// the format for everything
#define OUTFORMAT "%s[%.2f %.2f %.2f] [%s] [%s] [%s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
// what the battery warning says
#define WARN_LOW_BATT_TEXT "coliss"

// 10 seconds between iterations
#define SLEEP_INTERVAL 20

// locations of files
#define TEMPERATURE   "/sys/class/thermal/thermal_zone0/temp"
#define BATT_CAPACITY "/sys/class/power_supply/BAT0/capacity"
#define BATT_STATUS   "/sys/class/power_supply/BAT0/status"

#define COLO_RESET "" // reset
#define COLO_RED "" // red
#define COLO_YELLOW "" // yellow
#define COLO_DEEPGREEN "" // deep green
#define COLO_MAGENTA "" // magenta
#define COLO_CYAN "" // bright blue
#define COLO_BLUE "" // blue
