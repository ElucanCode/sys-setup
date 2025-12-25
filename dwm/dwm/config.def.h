/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;    /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;        /* 0 means no systray */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "monospace:size=15" };

/* For scheme pairs of 2 the first is the dark version */
static const int darkfirst = 1;
enum ColorScheme {
    Catppuccin_Mocha,
    Catppuccin_Latte,
    Default_Dwm,
    COLOR_SCHEMES_NUM,
};
static const char *colors[COLOR_SCHEMES_NUM][SchemeN][3] = {
		/*               fg         bg         border   */
	[Catppuccin_Mocha] = {
        [SchemeNorm] = { "#cdd6f4", "#1e1e2e", "#9399b2" },
        [SchemeSel]  = { "#1e1e2e", "#b4befe", "#b4befe"  },
	},
	[Catppuccin_Latte] = {
        [SchemeNorm] = { "#4c4f69", "#eff1f5", "#7c7f93" },
        [SchemeSel]  = { "#eff1f5", "#04a5e5", "#04a5e5"  },
	},
    [Default_Dwm] = {
        [SchemeNorm] = { "#bbbbbb", "#222222", "#444444" },
        [SchemeSel]  = { "#eeeeee", "#005577", "#005577"  },
    },
};

static const char *const autostart[] = {
    "dwmblocks", NULL,
    "lxsession", NULL, /* launches all .desktop files in ~/.config/autostart */
    NULL /* terminate */
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
    /* xprop(1):
     *    WM_CLASS(STRING) = instance, class
     *    WM_NAME(STRING) = title
     */
    /* class      instance    title       tags mask     isfloating   monitor */
    // { "Gimp",     NULL,       NULL,       0,            1,           -1 },
    { NULL, "launcher", NULL, 0, 1, -1 },
    { "ThisDoesNotExistAndIsSimplyAPlaceholder", NULL, NULL, 1, 0, -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },    /* first entry is default */
    { "[M]",      monocle },
    { "><>",      NULL },    /* no layout function means floating behavior */
    { NULL,       NULL },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
    { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
    { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
    { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
enum Cmd {
    Dmenu, Windows, Terminal, Browser, Filebrowser, Switch_Keyboard,
    Lockscreen,
    Volume_Up, Volume_Down, Volume_Toggle, Brightness_Up, Brightness_Down,
    Dwmblocks_Update_Volume, Dwmblocks_Update_Brightness,
};
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *cmds[][5] = {
    [Dmenu]           = { "rofi", "-show", "drun", NULL },
    [Windows]         = { "rofi", "-show", "window", NULL },
    [Terminal]        = { "alacritty", NULL },
    [Browser]         = { "librewolf", NULL },
    [Filebrowser]     = { "rofi", "-show", "filebrowser", NULL },
    [Switch_Keyboard] = { "rofi-switchkeyboard", NULL },
    [Lockscreen]      = { "betterlockscreen", "-l", "blur", NULL },
    [Brightness_Up]   = { "backlight_control", "+5%", NULL },
    [Brightness_Down] = { "backlight_control", "-5%", NULL },
    [Volume_Up]       = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL },
    [Volume_Down]     = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL },
    [Volume_Toggle]   = { "pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL },
    [Dwmblocks_Update_Brightness] = { "pkill", "-RTMIN+1", "dwmblocks", NULL },
    [Dwmblocks_Update_Volume] = { "pkill", "-RTMIN+2", "dwmblocks", NULL },
};

#include "movestack.c"

static const Key keys[] = {
    /* modifier                     key         function        argument */
    { MODKEY,             XK_p,                     spawn, {.v = cmds[Dmenu] } },
    { MODKEY|ControlMask, XK_w,                     spawn, {.v = cmds[Windows] } },
    { MODKEY,             XK_Return,                spawn, {.v = cmds[Terminal] } },
    { MODKEY,             XK_f,                     spawn, {.v = cmds[Filebrowser] } },
    { MODKEY|ControlMask, XK_k,                     spawn, {.v = cmds[Switch_Keyboard] } },
    { MODKEY|ControlMask, XK_l,                     spawn, {.v = cmds[Lockscreen] } },
    /* TODO: I don't like, that there is a duplication for these because dwmblocks needs to be updated*/
    { 0,                  XF86XK_MonBrightnessUp,   spawn, {.v = cmds[Brightness_Up] } },
    { 0,                  XF86XK_MonBrightnessDown, spawn, {.v = cmds[Brightness_Down] } },
    { 0,                  XF86XK_MonBrightnessUp,   spawn, {.v = cmds[Dwmblocks_Update_Brightness] } },
    { 0,                  XF86XK_MonBrightnessDown, spawn, {.v = cmds[Dwmblocks_Update_Brightness] } },
    { 0,                  XF86XK_AudioRaiseVolume,  spawn, {.v = cmds[Volume_Up] } },
    { 0,                  XF86XK_AudioLowerVolume,  spawn, {.v = cmds[Volume_Down] } },
    { 0,                  XF86XK_AudioMute,         spawn, {.v = cmds[Volume_Toggle] } },
    { 0,                  XF86XK_AudioRaiseVolume,  spawn, {.v = cmds[Dwmblocks_Update_Volume] } },
    { 0,                  XF86XK_AudioLowerVolume,  spawn, {.v = cmds[Dwmblocks_Update_Volume] } },
    { 0,                  XF86XK_AudioMute,         spawn, {.v = cmds[Dwmblocks_Update_Volume] } },

    { MODKEY,                       XK_b,       togglebar,      {0} },
    { MODKEY,                       XK_j,       focusstack,     {.i = +1 } },
    { MODKEY,                       XK_k,       focusstack,     {.i = -1 } },
    { MODKEY|ShiftMask,             XK_j,       movestack,      {.i = +1 } },
    { MODKEY|ShiftMask,             XK_k,       movestack,      {.i = -1 } },
    { MODKEY,                       XK_h,       setmfact,       {.f = -0.05} },
    { MODKEY,                       XK_l,       setmfact,       {.f = +0.05} },
    { MODKEY,                       XK_Return,  zoom,           {0} },
    { MODKEY,                       XK_Tab,     view,           {0} },
    { MODKEY|ShiftMask,             XK_c,       killclient,     {0} },
    { MODKEY,                       XK_n,       cyclelayout,    {.i = +1 } },
    { MODKEY|ShiftMask,             XK_n,       cyclelayout,    {.i = -1 } },
    { MODKEY|ShiftMask,             XK_space,   togglefloating, {0} },
    TAGKEYS(                        XK_1,                       0)
    TAGKEYS(                        XK_2,                       1)
    TAGKEYS(                        XK_3,                       2)
    TAGKEYS(                        XK_4,                       3)
    TAGKEYS(                        XK_5,                       4)
    TAGKEYS(                        XK_6,                       5)
    TAGKEYS(                        XK_7,                       6)
    TAGKEYS(                        XK_8,                       7)
    TAGKEYS(                        XK_9,                       8)
    { MODKEY|ControlMask|ShiftMask, XK_q,       quit,           {0} },
    { MODKEY|ShiftMask,             XK_r,       quit,           {1} }, /* restart */
    { MODKEY,                       XK_i,       incnmaster,     {.i = +1 } },
    { MODKEY,                       XK_d,       incnmaster,     {.i = -1 } },
    // { MODKEY,                       XK_t,       setlayout,      {.v = &layouts[0]} },
    // { MODKEY,                       XK_f,       setlayout,      {.v = &layouts[1]} },
    // { MODKEY,                       XK_m,       setlayout,      {.v = &layouts[2]} },
    // { MODKEY,                       XK_space,   setlayout,      {0} },
    // { MODKEY,                       XK_0,      view,           {.ui = ~0 } },
    // { MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
    { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
    { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
    { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
    { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
    /* click                event mask      button          function        argument */
    { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
    { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
    { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
    { ClkTagBar,            0,              Button1,        view,           {0} },
    { ClkTagBar,            0,              Button3,        toggleview,     {0} },
};

/* signal definitions */
/* signum must be greater than 0 */
/* trigger signals using `xsetroot -name "fsignal:<signum>"` */
static Signal signals[] = {
	/* signum       function        argument  */
	{ 1,            setlightscheme,      {0} },
	{ 2,            setdarkscheme,       {0} },
    { 3,            switchlightdark,     {0} },
};
