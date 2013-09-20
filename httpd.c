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
