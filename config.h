static char pwdir[]			= "/usr/local/share/dp"; /* pwdir - DP chdirs here */
static char csvpath[]		= "/usr/local/share/dp/modules.csv"; /* path to the modules file */
static char pidpath[]		= "/tmp/dp.pid"; /* path to DP' PID */

#ifdef DP /* only for Don't Panic Daemon */
static char notifycmd[]		= "notify-send";
#endif

#ifdef DPM /* only for Don't Panic Manager */
#define COLOR_PRIMARY	TB_WHITE	/* Primary color (eg. borders) */
#define COLOR_ACCENT 	TB_YELLOW	/* Accent color (eg. modules list) */
#endif
