#define LAYOUT_CMD    "setxkbmap -query | awk '/^layout/ { print $2 }'"
#define VOLUME_CMD    "pactl get-sink-volume @DEFAULT_SINK@ | awk '/^Volume/ { print $5 }'"

#define BACKLIGHT_DEV "amdgpu_bl1"
#define BACKLIGHT_CMD "cat /sys/class/backlight/" BACKLIGHT_DEV "/brightness | awk '{ print int($1/64764 * 100 + 0.5) }'"

#define BATTERY       "BAT0"
#define BATTERY_CMD   "cat /sys/class/power_supply/" BATTERY "/energy_now /sys/class/power_supply/" BATTERY "/energy_full | xargs echo | awk '{ print int($1/$2 * 100 + 0.5) }'"

#define TIME_CMD      "date '+%H:%M'"

#define DATE_CMD      "date '+%d.%m.%Y'"

static const Block blocks[] = {
    /*Icon*/    /*Command*/         /*Interval*/   /*Update Signal*/
    { " ",     LAYOUT_CMD,         60 * 60,        3 },
    { "󰕾 ",     VOLUME_CMD,         120,            2 },
    { "󰃞 ",     BACKLIGHT_CMD,      120,            1 },
    { "󰁹 ",     BATTERY_CMD,        60,             0 },
    { "󰅐 ",     TIME_CMD,           10,             0 },
    { "󰃭 ",     DATE_CMD,           600,            0 },
};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = " | ";
static unsigned int delimLen = 5;
