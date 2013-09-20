/*  mini-httpd v0.1
 *  Tiny httpd written in C for Linux.
 *  Copyright (C) 2013  Peter Malone
 *  Licensed under GPLv2. See the LICENSE file for details.
 * 
 * 
 *  TODO LIST:
 *          1. Clean code and modularize
 *          2. Add logger
 *          3. Add threading
 *          4. Add signals
 *          5. Add HTTP POST
 *          6. Secure, secure, secure!
 * 
 */

#include <stdio.h>
#include <stdlib.h>      // used for exit(int)
#include <unistd.h>      // used for fopen(char *, char*)
#include <syslog.h>
#include <sys/types.h>   // used for pid_t
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
void read_config(char *);
void daemonize();
void enable_syslog();
void sockify();
void parseWebRequest(char *, int, int);
void open_log(char *, char *);
void terminate(const char *, ...);
typedef struct {
        unsigned short int port;
        char webroot[128];
        char logdir[128];
        char log[128];
} config;
config conf;
char * conffile = "httpd.conf";
char * homedir = NULL;
char * webroot_fullpath = NULL;
char * logdir_fullpath = NULL;
FILE * logfile = NULL;
pid_t pid, sid;
int listenfd = 0, connfd = 0;
struct sockaddr_in serv_addr;
char sendBuff[8192];
char recvBuff[8192];
int r, s, t;
int main(void)
{
        enable_syslog();
        read_config(conffile);
	open_log(logdir_fullpath, conf.log);
        daemonize();
        sockify();
        closelog();
        return 0;
}

/* read config
 *  added steps to validate the user input
 *  both WEBROOT and LOGDIR are added to homedir. If those directories don't exist under homedir, we exit.
 *  LOG is opened as LOGDIR + LOG. we create the file if it doesn't exist.
 *  we use fscanf (from stdio.h) to read it
 */
void read_config(char * f)
{
        FILE * cfile;
	char * str = NULL;
	char *slash = "/";
        struct passwd *pw = getpwuid(getuid());
        int BUFSIZE = 128;
        char *webroot = NULL;
        char *logdir = NULL;
        char *log = NULL;
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
        cfile = fopen(f, "r+");

        if(cfile == NULL) {
                terminate("Fatal Error! Error opening config file");
        }
        else {
                printf("Config file: %s successfully opened!\n", f);
                if(fscanf(cfile, "PORT=%d\nWEBROOT=%127s\nLOGDIR=%127s\nLOG=%127s", &conf.port, conf.webroot, conf.logdir, conf.log) == 4) {
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
void enable_syslog()
{
        openlog("httpd", LOG_USER| LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
        syslog(LOG_INFO, "httpd started by user %d", getuid());
}

/* fork process
 * become session leader
 * fork again
 * clear file mode creation mask
 * chdir and close file descriptors
 * fork process twice
 * then chdir and open port
 */
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
void sockify()
{
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd < 0) {
                terminate("Error opening socket. %s", strerror(errno));
        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        memset(sendBuff, '0', sizeof(sendBuff));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(conf.port);
        t = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if( t < 0) {
                terminate("Error binding port. %s", strerror(errno));

        }
        t = listen(listenfd, 10);
        if( t < 0) {
                syslog(LOG_ERR, "Error listening on socket. %s", strerror(errno));
                exit(EXIT_FAILURE);

        }


        while(1)
        {
                connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
                if(connfd >= 0) {
                        bzero(recvBuff,sizeof(recvBuff));
                        r = read(connfd, recvBuff,sizeof(recvBuff));
                        if(r < 0) {
                                syslog(LOG_ERR,"Issue reading buffer. Error is: %s", strerror(errno));
                        }
                        else {
                                parseWebRequest(recvBuff, connfd, r);
                                /*s = write(connfd, recvBuff, sizeof(recvBuff));*/
                        }
                        if(s < 0) {
                                syslog(LOG_ERR,"Issue writing socket. Error is: %s", strerror(errno));
                        }

                        close(connfd);
                        sleep(1);

                }
                else {

                        syslog(LOG_ERR, "Issue accepting connection");
                        close(connfd);
                }
        }

}

void parseWebRequest(char * req, int sock, int num_read)
{
        int fd, nread,i;
        FILE *hfile = NULL;
        char *fullpath, method[128], url[128], buf[1024], protocol[128], htmlfile[128];
        struct stat file_stats;
        char * str;
        char * ok_response =
                "HTTP/1.0 200 OK\n"
                "Content-type: text/html\n"
                "\n";
        char * bad_request_response =
                "HTTP/1.0 400 Bad Request\n"
                "Content-type: text/html\n"
                "\n"
                "<html>\n"
                " <body>\n"
                "  <h1>Bad Request</h1>\n"
                "  <p>This server did not understand your request.</p>\n"
                " </body>\n"
                "</html>\n";

        char * not_found_response =
                "HTTP/1.0 404 Not Found\n"
                "Content-type: text/html\n"
                "\n"
                "<html>\n"
                " <body>\n"
                "  <h1>Not Found</h1>\n"
                "  <p>The requested URL was not found on this server.</p>\n"
                " </body>\n"
                "</html>\n";

        char * verboten_response =
                "HTTP/1.0 403 Forbidden\n"
                "Content-type: text/html\n"
                "\n"
                "<html>\n"
                " <body>\n"
                "  <h1>Forbidden</h1>\n"
                "  <p>You are not permitted to view the requested URL on this server.</p>\n"
                " </body>\n"
                "</html>\n";

        char * bad_method_response =
                "HTTP/1.0 501 Method Not Implemented\n"
                "Content-type: text/html\n"
                "\n"
                "<html>\n"
                " <body>\n"
                "  <h1>Method Not Implemented</h1>\n"
                "  <p>This method is not implemented by this server.</p>\n"
                " </body>\n"
                "</html>\n";
        fprintf(logfile, "Full request is as follows: %s\n", req);
        fflush(logfile);
        sscanf(req, "%127s %127s %127s", method, url, protocol);
        if (strcmp (protocol, "HTTP/1.0") && strcmp (protocol, "HTTP/1.1")) {
                /* We don't understand this protocol.  Report a bad response.  */
                fprintf(logfile, "We did not understand the request.\n");
                fflush(logfile);
                write (sock, bad_request_response, strlen (bad_request_response));
        }
        else if (strcmp (method, "GET")) {
        /* This server only implements the GET method.  The client
 	 * specified some other method, so report the failure.  */
                char response[1024];

                snprintf (response, sizeof (response), bad_method_response, method);
                write (sock, response, strlen (response));
        }
        /* Prevent directory traversal */
        else if((str = strstr(url, "..")) != NULL || (str = strstr(url, "%2E%2E")) != NULL || (str = strstr(url, "\056\056")) != NULL) {
                fprintf(logfile, "Bad request. File contained ..\n");
                fflush(logfile);
                write (sock, bad_request_response, strlen (bad_request_response));
        }
        else {
                /* A valid request.  Process it.  */
                if(strncmp(url, "/\0", 2)==0) {
                        strncpy(htmlfile, "/index.html", sizeof(htmlfile));
                        fprintf(logfile, "html file is %s\n", htmlfile);
                        fflush(logfile);

                        fullpath = malloc(snprintf(NULL, 0, "%s%s", webroot_fullpath, htmlfile) + 1);
                        sprintf(fullpath, "%s%s", webroot_fullpath, htmlfile);
                        fprintf(logfile, "full path is %s\n", fullpath);
                        fflush(logfile);

                }
                else {
                        strncpy(htmlfile, url, sizeof(htmlfile));
                        fullpath = malloc(snprintf(NULL, 0, "%s%s", webroot_fullpath, htmlfile) + 1);
                        sprintf(fullpath, "%s%s", webroot_fullpath, htmlfile);
                        fprintf(logfile, "full path is %s\n", fullpath);
                        fflush(logfile);


                }

                if((stat(fullpath, &file_stats)) == -1) {
                        /*Unable to find file*/
                        fprintf(logfile, "Error finding: %s\n", fullpath);
                        fflush(logfile);
                        write (sock, not_found_response, strlen (not_found_response));
                }
		/* check if request is for directory 
		 * TODO handle this properly. We should display directory contents dynamically and allow the user to navigate
		 */
                else { 
			if(S_ISDIR(file_stats.st_mode)) {
				fprintf(logfile, "Bad request. User requested a directory\n");
				fflush(logfile);
				write (sock, bad_request_response, strlen (bad_request_response));
			}
			/*TODO handle this properly. For now we assume we are a file*/ 
			else {
                        	fprintf(logfile, "File exists. \n");
                        	fflush(logfile);
                        	hfile = fopen(fullpath, "r");
                        	if(hfile == NULL) {
                                	fprintf(logfile, "Error opening: %s\n", fullpath);
                                	fflush(logfile);
                                	write (sock, verboten_response, strlen (verboten_response));
                        	}
                        	else {
                                	/*read file contents into buffer and send back to client*/
                                	fseek(hfile, 0, SEEK_END);
                                	long fsize = ftell(hfile);
                                	fseek(hfile, 0, SEEK_SET);

                                	char *string = malloc(fsize + 1);
                                	fread(string, fsize, 1, hfile);
                                	fclose(hfile);

                                	string[fsize] = 0;
                                	write (sock, ok_response, strlen (ok_response));
                                	write (sock, string, strlen(string));

                        	}
			}
                }
        }
}

void open_log(char * ld, char * l)
{
        if ((chdir(ld)) < 0) {
                terminate("Failed to chdir to %s", conf.webroot);
        }

        logfile = fopen(conf.log, "a+");

}
void terminate(const char * msg, ...) {

        va_list ap;
        va_start(ap, msg);
        vsyslog(LOG_ERR, msg, ap);
        va_end(ap);
        closelog();
        exit(EXIT_FAILURE);


}
