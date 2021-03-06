#include "sock.h"
#include "config.h"
#include "web.h"
#include "log.h"
FILE * logfile = NULL;
void parseWebRequest(char * req, int sock, int num_read)
{
        FILE *hfile = NULL;
        char *fullpath, method[128], url[128], protocol[128], htmlfile[128];
        struct stat file_stats;
        char * str;
	long fsize = 0;
	char * string = NULL;
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
        wlog(logfile, "Full request is as follows: %s\n", req);
        sscanf(req, "%127s %127s %127s", method, url, protocol);
        if (strcmp (protocol, "HTTP/1.0") && strcmp (protocol, "HTTP/1.1")) {
                /* We don't understand this protocol.  Report a bad response.  */
                wlog(logfile, "We did not understand the request.\n");
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
                wlog(logfile, "Bad request. File contained ..\n");
                write (sock, bad_request_response, strlen (bad_request_response));
        }
        else {
                /* A valid request.  Process it.  */
                if(strncmp(url, "/\0", 2)==0) {
                        strncpy(htmlfile, "/index.html", sizeof(htmlfile) -1);
                        wlog(logfile, "html file is %s\n", htmlfile);

                        fullpath = malloc(snprintf(NULL, 0, "%s%s", webroot_fullpath, htmlfile) + 1);
                        sprintf(fullpath, "%s%s", webroot_fullpath, htmlfile);
                        wlog(logfile, "full path is %s\n", fullpath);

                }
                else {
                        strncpy(htmlfile, url, sizeof(htmlfile) -1);
                        fullpath = malloc(snprintf(NULL, 0, "%s%s", webroot_fullpath, htmlfile) + 1);
                        sprintf(fullpath, "%s%s", webroot_fullpath, htmlfile);
                        wlog(logfile, "full path is %s\n", fullpath);


                }

                if((stat(fullpath, &file_stats)) == -1) {
                        /*Unable to find file*/
                        wlog(logfile, "Error finding: %s\n", fullpath);
                        write (sock, not_found_response, strlen (not_found_response));
                }
		/* check if request is for directory 
  		 * TODO handle this properly. We should display directory contents dynamically and allow the user to navigate
  		 */
                else { 
			if(S_ISDIR(file_stats.st_mode)) {
				wlog(logfile, "Bad request. User requested a directory\n");
				write (sock, bad_request_response, strlen (bad_request_response));
			}
			/*TODO handle this properly. For now we assume we are a file*/ 
			else {
                        	wlog(logfile, "File exists. \n");
                        	hfile = fopen(fullpath, "r");
                        	if(hfile == NULL) {
                                	wlog(logfile, "Error opening: %s\n", fullpath);
                                	write (sock, verboten_response, strlen (verboten_response));
                        	}
                        	else {
                                	/*read file contents into buffer and send back to client*/
                                	fseek(hfile, 0, SEEK_END);
                                	fsize = ftell(hfile);
                                	fseek(hfile, 0, SEEK_SET);

                                	string = malloc(fsize + 1);
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

