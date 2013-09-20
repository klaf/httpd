#include "log.h"
#include "terminate.h"
#include "config.h"
void enable_syslog()
{
        openlog("httpd", LOG_USER| LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
        syslog(LOG_INFO, "httpd started by user %d", getuid());
}

void open_log(char * ld, char * l)
{
        if ((chdir(ld)) < 0) {
                terminate("Failed to chdir to %s", conf.webroot);
        }

        logfile = fopen(conf.log, "a+");

}

