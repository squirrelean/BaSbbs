#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bbfile.h"
#include "server.h"

#include "globals.h"
#include "read_config.h"

ServerConfig global_config;

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    global_config = (ServerConfig){.thmax = 25, .thincr = 5, .bbport = 9000, .fdebug = false, .bbfile = NULL};

    char *config_path;
    if (argc == 2)
        config_path = argv[1];
    else
        config_path = "bbserv.conf";

    read_config_file(&global_config, config_path);
    if (!global_config.bbfile) {
        printf("BBFILE required\n");
        exit(EXIT_FAILURE);
    }

    printf("thmax: %d, thincr: %d, bbport: %d, fdebug: %d, bbfile: %s\n", global_config.thmax,
           global_config.thincr, global_config.bbport, global_config.fdebug, global_config.bbfile);

    bb_init();

    initialize_server();

    free(global_config.bbfile);
}
