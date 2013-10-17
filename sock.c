#include "sock.h"
#include "config.h"
#include "terminate.h"
#include "web.h"
#include <pthread.h>
void processConnection(void * arg)
{

	int r;
	char recvBuff[8192];
	int *conn = (int*) arg;
	bzero(recvBuff,sizeof(recvBuff));

	r = read(*conn, recvBuff,sizeof(recvBuff));
	if(r < 0) {
		     syslog(LOG_ERR,"Issue reading buffer. Error is: %s", strerror(errno));
	}
	else {
			parseWebRequest(recvBuff, *conn, r);

	}

        close(*conn);
//	pthread_exit((void*)conn);
	pthread_exit(0);
//	sleep(1);



}

void sockify()
{
	int listenfd, connfd;
	struct sockaddr_in serv_addr;
	int t;

	pthread_t thread;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if(listenfd < 0) {
                terminate("Error opening socket. %s", strerror(errno));
        }

        memset(&serv_addr, '0', sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(conf.port);
        t = bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        if( t < 0) {
                terminate("Error binding port. %s", strerror(errno));

        }
        t = listen(listenfd, 100);
        if( t < 0) {
                syslog(LOG_ERR, "Error listening on socket. %s", strerror(errno));
                exit(EXIT_FAILURE);

        }


        while(1)
        {
               if ( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL) ) >=0) {
                  	pthread_create(&thread, NULL, (void *) processConnection, (void*) &connfd);
			pthread_join(thread, NULL);
		/*	bzero(recvBuff,sizeof(recvBuff));
                        r = read(connfd, recvBuff,sizeof(recvBuff));
                        if(r < 0) {
                                syslog(LOG_ERR,"Issue reading buffer. Error is: %s", strerror(errno));
                        }
                        else {
                                parseWebRequest(recvBuff, connfd, r);
                        }

                        close(connfd);
                        sleep(1);
		*/
	       }
                else {

                        syslog(LOG_ERR, "Issue accepting connection");
                        close(connfd);
                }
        }

}
