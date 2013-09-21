#include "includes.h"
typedef struct {
        unsigned short int port;
        char webroot[128];
        char logdir[128];
        char log[128];
} config;
extern config conf;
extern char * homedir;
extern char * webroot_fullpath;
extern char * logdir_fullpath;
extern FILE * logfile;
void read_config();

