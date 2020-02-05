#pragma once

/* SETTINGS */

// the format for the date/time
#define TIME_STR_FMT "%a %d %b  %H:%M"
// how many chars to allocate for the date/time
#define TIME_STR_LEN 32
// how many chars to allocate for the battery
#define BATT_STR_LEN 16
// the format for everything
#define OUTFORMAT "%s[%.2f %.2f %.2f] [%s] [%s] [%s] %s"
// level to warn on low battery
#define WARN_LOW_BATT 9
// what the battery warning says
#define WARN_LOW_BATT_TEXT "coliss"
// how many chars to allocate for the network string
#define MAX_NET_MSG_LEN 64

// how many seconds between iterations
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
