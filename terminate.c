#include "terminate.h"
void terminate(const char * msg, ...) 
{

        va_list ap;
        va_start(ap, msg);
        vsyslog(LOG_ERR, msg, ap);
        va_end(ap);
        closelog();
        exit(EXIT_FAILURE);


}
