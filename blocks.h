#define PATHTOCMD "/home/william/.config/dwm/statscripts/"

#define CMD(cmd) PATHTOCMD #cmd ".sh"

//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/	/*Command*/		/*Update Interval*/	/*Update Signal*/
	{"🧠 ", CMD("cpu"),	                1,			1},
	{"🌡️ ", CMD("temp"),	                1,			8},
	{"📝 ", CMD("memory"),	                1,			2},
	{"🔊 ", CMD("volume"),	                1,      		10},
	{"🎤 ", CMD("mic"),	                1,      		13},
	{"🌐 ", CMD("network"),                 30,      		3},
	{"🔒 ", CMD("vpn"),                     1,      		15},
	{"🎧 "  ,CMD("bluetooth"),              1,      		4},
	{"🖥️ ",   CMD("backlight"),            0,      		5},
	{"⛏️ "  ,CMD("miner"),                   1,      		14},
	{"⌨️ "  ,CMD("keylayout"),               1,      		6},
	{"📅 ",CMD("date"),	                60,			7},
	{"📻",  CMD("player"),	                0,			11},
	{"💿",CMD("disk"),	                0,			12},
	{"🔌",CMD("power"),	                0,			9},
};

//sets delimeter between status commands. NULL character ('\0') means no delimeter.
static char delim[] = "  ";
static unsigned int delimLen = 2;
static unsigned int padding = 2;

static double timeout = 0.1;
