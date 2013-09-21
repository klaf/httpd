#include "sock.h"
#include "config.h"
#include "terminate.h"
#include "web.h"
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

                        close(connfd);
                        sleep(1);

                }
                else {

                        syslog(LOG_ERR, "Issue accepting connection");
                        close(connfd);
                }
        }

}
