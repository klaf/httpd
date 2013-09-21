#include <time.h>
#include <sys/time.h>
#include "log.h"
#include "terminate.h"
#include "config.h"

/* open syslog, log format will be "httpd[pid] msg"*/
void enable_syslog()
{
        openlog("httpd", LOG_USER| LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
        syslog(LOG_INFO, "httpd started by user %d", getuid());
}
/* open logfile. logfile is read from our config file. see config.h and config.c */
void open_log()
{
        if ((chdir(conf.webroot)) < 0) {
                terminate("Failed to chdir to %s", conf.webroot);
        }

        logfile = fopen(conf.log, "a+");

}
/* when we daemonize we close the std file descriptors. as a result we need to fflush our log file when we write to it
 * this is a wrapper function to write and flush. 
 * this function also gets the current timestamp and writes it to the log
 * ctime returns \n so we remove that from the string in order for the timestamp to be on the same line as the message we are logging
 */
void wlog(FILE * f, const char * msg, ...) 
{
	char * curtime = NULL;
	char * line = NULL;
	size_t len = 0;
	va_list ap;
	time_t clk = time(NULL);
    	curtime = ctime(&clk);
	len = strlen(curtime) -1;
	if (curtime[len] == '\n') curtime[len] = '\0';
	line = malloc(snprintf(NULL, 0, "%s %s", curtime, msg) + 1);
        sprintf(line, "%s %s", curtime, msg);
        va_start(ap, msg);
        vfprintf(f, line, ap);
	va_end(ap);
        fflush(f);
}
