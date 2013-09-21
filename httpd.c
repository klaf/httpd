/*  mini-httpd v0.2
 *  Tiny httpd written in C for Linux.
 *  Copyright (C) 2013  Peter Malone
 *  Licensed under GPLv2. See the LICENSE file for details.
 * 
 * 
 *  v0.3 TODO LIST:
 *          	1. Add threading
 *          	2. Add signals
 *          	3. Add HTTP POST
 *          	4. Secure, secure, secure!
 *
 */

#include "includes.h"
#include "config.h"
#include "terminate.h"
#include "daemon.h"
#include "log.h"
#include "web.h"
#include "sock.h"
int main(void)
{ 
	enable_syslog();
        read_config(conffile);
	open_log(logdir_fullpath);
        daemonize();
        sockify();
        closelog();
        return 0;
}
