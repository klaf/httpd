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
