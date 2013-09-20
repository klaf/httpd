#include "includes.h"

int listenfd, connfd;
struct sockaddr_in serv_addr;
char sendBuff[8192];
char recvBuff[8192];
int r, s, t;


void sockify(void);
