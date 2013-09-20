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
void open_log(char * ld, char * l)
{
        if ((chdir(ld)) < 0) {
                terminate("Failed to chdir to %s", conf.webroot);
        }

        logfile = fopen(conf.log, "a+");

}
/* when we daemonize we close the std file descriptors. as a result we need to fflush our log file when we write to it
 * this is a wrapper function to write and flush. 
 */
void wlog(FILE * f, const char * msg, ...) 
{

        va_list ap;
        va_start(ap, msg);
        vfprintf(f, msg, ap);
        fflush(f);
}
