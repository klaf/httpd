#include "includes.h"
#include "terminate.h"
#include "config.h"

const char * conffile = "httpd.conf";
char * homedir = NULL;
char * webroot_fullpath = NULL;
char * logdir_fullpath = NULL;
config conf;

/* read config
 *  added steps to validate the user input
 *  both WEBROOT and LOGDIR are added to homedir. If those directories don't exist under homedir, we exit.
 *  LOG is opened as LOGDIR + LOG. we create the file if it doesn't exist.
 *  we use fscanf (from stdio.h) to read it
 */
void read_config()
{
        FILE * cfile;
	char * str = NULL;
	char *slash = "/";
        struct passwd *pw = getpwuid(getuid());
/*        char *webroot = NULL;
        char *logdir = NULL;
        char *log = NULL;*/
	struct stat file_stats;
        if (pw) {
                homedir = pw->pw_dir;
        }
        else {
                terminate("Fatal Error! Could not read home directory. Exiting. :(");
        }
        if(homedir == NULL) {
                terminate("Fatal Error! Could not read pw->pw_dir.\n");
        }
        cfile = fopen(conffile, "r+");

        if(cfile == NULL) {
                terminate("Fatal Error! Error opening config file");
        }
        else {
                printf("Config file: %s successfully opened!\n", conffile);
                if(fscanf(cfile, "PORT=%hu\nWEBROOT=%127s\nLOGDIR=%127s\nLOG=%127s", &conf.port, conf.webroot, conf.logdir, conf.log) == 4) {
                        if(conf.port < 1025 && conf.port > 65534) {
                                terminate("Fatal Error! port number not in range 1025 - 65534. Choose another port. :(");
                        }
                        printf("Found port number in config, it's %d\n", conf.port);
			if(conf.webroot == NULL || conf.logdir == NULL || conf.log == NULL) {
				terminate("Fatal Error! Conf variables set incorrectly..\n");
			}
			else if((str = strstr(conf.webroot, "..")) != NULL || (str = strstr(conf.logdir, "..")) != NULL || (str = strstr(conf.log, "..")) != NULL) {
				terminate("Fatal Error! Config variables contain periods.\n");
			}
			else if((str = strstr(conf.webroot, " ")) != NULL || (str = strstr(conf.logdir, " ")) != NULL || (str = strstr(conf.log, " ")) != NULL) {	
				terminate("Fatal Error! Config variables contain spaces.\n");
			}

	                /* validate conf.webroot & conf.logdir
   			 * must be subdirectory of homedir, (stat homedir + conf.webroot and homedir + conf.logdir) otherwise we assume malicious intent and exit
   			 */

			/* ensure webroot_fullpath is big enough to handle homedir + webroot */
			webroot_fullpath = malloc(snprintf(NULL, 0, "%s%s%s", homedir, slash, conf.webroot) + 1);
                        sprintf(webroot_fullpath, "%s%s%s", homedir, slash, conf.webroot);
			printf("webrootfull is %s\n", webroot_fullpath);
			if((stat(webroot_fullpath, &file_stats)) == -1) {
				terminate("Fatal error! %s not found in %s\n", conf.webroot, homedir);
			}
			/* ensure logdir_fullpath is big enough to handle homedir + logdir */
			logdir_fullpath = malloc(snprintf(NULL, 0, "%s%s%s", homedir, slash, conf.logdir) + 1);
			sprintf(logdir_fullpath, "%s%s%s", homedir, slash, conf.logdir);
			printf("logdir is %s\n", logdir_fullpath);
			if((stat(logdir_fullpath, &file_stats)) == -1) {
                                terminate("Fatal Error! %s not found in %s\n", conf.logdir, homedir);
                        }
             }
             else {
                        terminate("Fatal Error! Error reading config file. Exiting. :(");
                  }
        }
        /* if we get here, we're done loading the config file so lets close it */
        fclose(cfile);

}
