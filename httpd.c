/*  mini-httpd v0.1 
 *  Tiny httpd written in C for Linux.
 *  Copyright (C) 2013  Peter Malone
 *  Licensed under GPLv2. See the LICENSE file for details.
 *
 *
 * TODO LIST:
 *		1. Add support for $HOME instead of hard linking to /home/sj/
 *		2. Clean code and modularize
 *      	3. Add logger
 *		4. Add threading
 *		5. Add signals
 *		6. Add HTTP POST
 *		7. Secure, secure, secure!
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
void read_config(char *);
void daemonize();
void enable_syslog();
void sockify();
void parseWebRequest(char *, int, int);
void open_log(char *, char *);
typedef struct {
        unsigned short int port;
        char webroot[128];
        char logdir[128];
        char log[128];
} config;
config conf;
char * conffile = "httpd.conf";
FILE * logfile = NULL;
pid_t pid, sid;
int listenfd = 0, connfd = 0;
struct sockaddr_in serv_addr;
char sendBuff[8192];
char recvBuff[8192];
int r, s, t;
int main(void) {
        enable_syslog();
        read_config(conffile);
	open_log(conf.logdir, conf.log);
        daemonize();
        sockify();
        closelog();
        return 0;

}

/* read config
 *  
 * we use fscanf (from stdio.h) to read it
 * 
 */
void read_config(char * f) {
        FILE * cfile;
        cfile = fopen(f, "r+");
        if(cfile == NULL){
                syslog(LOG_ERR, "Error opening config file %s", f);
                closelog();
                exit(EXIT_FAILURE);
        }
        else {
                printf("Config file: %s successfully opened!\n", f);
                if(fscanf(cfile, "PORT=%d\nWEBROOT=%128s\nLOGDIR=%128s\nLOG=%128s", &conf.port, conf.webroot, conf.logdir, conf.log) == 4) {
                        if(conf.port > 8888) {
                                syslog(LOG_ERR, "Error! port number is greater than 8888. Exiting. :(");
                                closelog();
                                fclose(cfile);
                                exit(EXIT_FAILURE);
                        }
                        printf("Found port number in config, it's %d\n", conf.port);
                        printf("Found directory name in config, it's %s\n",conf.webroot);
             }
             else {
                        syslog(LOG_ERR, "Error reading config file. Exiting. :(");
                        closelog();
                        fclose(cfile);
                        exit(EXIT_FAILURE);
                  }
        }
        /* if we get here, we're done loading the config file so lets close it */
        fclose(cfile);

}
void enable_syslog() {
        openlog("httpd", LOG_USER| LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL0);
        syslog(LOG_INFO, "httpd started by user %d", getuid());
}

/* fork process
 *  * become session leader
 *   * fork again
 *    * clear file mode creation mask
 *     * chdir and close file descriptors
 *      * fork process twice
 *       * then chdir and open port
 *        */
void daemonize() {
        /* Fork from the parent process */
        pid = fork();
        if (pid < 0) {
                syslog(LOG_ERR, "Failed to fork(). Exiting. :(");
                closelog();
                exit(EXIT_FAILURE);
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
                syslog(LOG_ERR, "SID creation failed. Exiting. :(");
                closelog();
                exit(EXIT_FAILURE);
        }

        /* lets fork again to ensure we are not session leader */
        pid = fork();
        if (pid < 0) {
                syslog(LOG_ERR, "Failed to fork(). Exiting. :(");
                closelog();
                exit(EXIT_FAILURE);
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
        if ((chdir(conf.webroot)) < 0) {
                syslog(LOG_ERR, "Failed to chdir to %s", conf.webroot);
                perror("chdir");
                closelog();
                exit(EXIT_FAILURE);
        }
        syslog(LOG_INFO, "We should be a daemon now.");
        /* Close out the standard file descriptors */


        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);


}
void sockify() {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd < 0) {
                syslog(LOG_ERR, "Error opening socket. %s", strerror(errno));
                exit(EXIT_FAILURE);

        }

        memset(&serv_addr, '0', sizeof(serv_addr));
        memset(sendBuff, '0', sizeof(sendBuff));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(conf.port);
        t = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if( t < 0) {
                syslog(LOG_ERR, "Error binding port. %s", strerror(errno));
                exit(EXIT_FAILURE);

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

void parseWebRequest(char * req, int sock, int num_read){
        int fd, nread,i;
        FILE *hfile = NULL;
        char *fullpath, *reqline[3], method[128], url[128], buf[1024], protocol[128], htmlfile[128];
        struct stat file_stats;
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
        sscanf(req, "%s %s %s", method, url, protocol);
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
    else {
      /* A valid request.  Process it.  */
        if(strncmp(url, "/\0", 2)==0 ) {
                strcpy(htmlfile, "/index.html");
                fprintf(logfile, "html file is %s\n", htmlfile);
                fflush(logfile);

                fullpath = malloc(snprintf(NULL, 0, "%s%s", conf.webroot, htmlfile) + 1);
                sprintf(fullpath, "%s%s", conf.webroot, htmlfile);
                fprintf(logfile, "full path is %s\n", fullpath);
                fflush(logfile);

        }
        else {
                strncpy(htmlfile, url, sizeof(htmlfile));
                fullpath = malloc(snprintf(NULL, 0, "%s%s", conf.webroot, htmlfile) + 1);
                sprintf(fullpath, "%s%s", conf.webroot, htmlfile);
                fprintf(logfile, "full path is %s\n", fullpath);
                fflush(logfile);


        }


        if((stat(fullpath, &file_stats)) == -1) {
                /*Unable to find file*/
                fprintf(logfile, "Error finding: %s\n", fullpath);
                fflush(logfile);
                write (sock, not_found_response, strlen (not_found_response));
        }
        else {

                fprintf(logfile, "File exists. \n");
                fflush(logfile);
                hfile = fopen(fullpath, "r");
                if(hfile == NULL){
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
		
void open_log(char * ld, char * l){
        if ((chdir(ld)) < 0) {
                syslog(LOG_ERR, "Failed to chdir to %s", conf.webroot);
                perror("chdir");
                closelog();
                exit(EXIT_FAILURE);
        }

        logfile = fopen(conf.log, "a+");
	
}
