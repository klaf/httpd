#include "includes.h"

int listenfd, connfd;
struct sockaddr_in serv_addr;
int s, t;


void sockify(void);
void processConnection(void *);
