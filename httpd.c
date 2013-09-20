/*  mini-httpd v0.1
 *  Tiny httpd written in C for Linux.
 *  Copyright (C) 2013  Peter Malone
 *  Licensed under GPLv2. See the LICENSE file for details.
 * 
 * 
 *  v0.2 TODO LIST:
 *          	1. Add logger
 *          	2. Add threading
 *          	3. Add signals
 *          	4. Add HTTP POST
 *          	5. Secure, secure, secure!
 *
 */

#include "includes.h"
#include "config.h"
#include "terminate.h"
#include "daemon.h"
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
