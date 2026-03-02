#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "server.h"

#include "read_config.h"
int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    ServerConfig config = {.thmax = 25, .thincr = 5, .bbport = 9000, .fdebug = false, .bbfile = NULL};

    char *config_path;
    if (argc == 2)
        config_path = argv[1];
    else
        config_path = "bbserv.conf";

    read_config_file(&config, config_path);
    if (!config.bbfile) {
        printf("BBFILE required\n");
        exit(EXIT_FAILURE);
    }

    printf("thmax: %d, thincr: %d, bbport: %d, fdebug: %d, bbfile: %s", config.thmax, config.thincr,
           config.bbport, config.fdebug, config.bbfile);

    initialize_server(&config);

    free(config.bbfile);
}
