#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bbfile.h"
#include "server.h"

#include "globals.h"
#include "read_config.h"

ServerConfig global_config;
ReplicationConfig global_rconfig;

void print_config();

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    global_config = (ServerConfig){.thmax = 25, .thincr = 5, .bbport = 9000, .fdebug = false, .bbfile = NULL};
    global_rconfig = (ReplicationConfig){.fground = 0, .pdebug = 0, .rport = 9001, .peer = NULL};

    char *config_path;
    if (argc == 2)
        config_path = argv[1];
    else
        config_path = "bbserv.conf";

    read_config_file(&global_config, &global_rconfig, config_path);
    if (!global_config.bbfile) {
        printf("BBFILE required\n");
        exit(EXIT_FAILURE);
    }

    print_config();

    bb_init();

    initialize_server();

    free(global_config.bbfile);
    free_peers(global_rconfig.peer, global_rconfig.peer_count);
}

void print_config()
{
    printf("thmax: %d, thincr: %d, bbport: %d, fdebug: %d, bbfile: %s\n", global_config.thmax,
           global_config.thincr, global_config.bbport, global_config.fdebug, global_config.bbfile);

    printf("forground: %d, pdebug: %d, rport: %d\n", global_rconfig.fground, global_rconfig.pdebug,
           global_rconfig.rport);

    for (int i = 0; i < global_rconfig.peer_count; i++)
        printf("peer #%d. Host: %s, Port: %d\n", i, global_rconfig.peer[i].host, global_rconfig.peer[i].port);
}
