#include "daemon.h"
#include "config.h"
/* fork process
 *  * become session leader
 *   * fork again
 *    * clear file mode creation mask
 *     * chdir and close file descriptors
 *      * fork process twice
 *       * then chdir and open port
 *        */
void daemonize()
{
        /* Fork from the parent process */
        pid = fork();
        if (pid < 0) {
                terminate("Failed to fork(). Exiting. :(");
        }
        /* If we have a good PID, then we can exit the parent process. */
        if (pid > 0) {
                syslog(LOG_INFO, "We have a good pid. Exiting parent.");
                closelog();
                exit(EXIT_SUCCESS);
        }

        /* Become session leader */
        sid = setsid();
        if (sid < 0) {
                terminate("SID creation failed. Exiting. :(");
        }

        /* lets fork again to ensure we are not session leader */
        pid = fork();
        if (pid < 0) {
        	terminate("Failed to fork(). Exiting. :(");
	}
        /* If we have a good PID, then we can exit the parent process. */
        if (pid > 0) {
                syslog(LOG_INFO, "We have a good pid. Exiting parent.");
                closelog();
                exit(EXIT_SUCCESS);
        }


        /* Clear file mode creation mask */
        umask(0);

        /* Change to the directory given in config file */
        if ((chdir(webroot_fullpath)) < 0) {
                terminate("Failed to chdir to %s", webroot_fullpath);
        }
        syslog(LOG_INFO, "We should be a daemon now.");
        /* Close out the standard file descriptors */


        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);


}
