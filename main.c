#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbfile.h"
#include "server.h"

#include "daemonize.h"
#include "globals.h"
#include "read_config.h"
#include "signal.h"

ServerConfig global_config;
ReplicationConfig global_rconfig;

void print_config();
void free_allocated_memory();
void configure_server(char *config_path);
char *resolve_path(char *path);

int main(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    initialize_signals();

    char *config_path = NULL;
    if (argc == 2)
        config_path = strdup(argv[1]);
    else
        config_path = resolve_path("bbserv.conf");

    configure_server(config_path);

    const char *pidfile = "bbserv.pid_file";
    int pidfd;
    char *absolute_bbfile;
    if (!global_rconfig.fground) {
        printf("Server running daemonized\n");

        absolute_bbfile = resolve_path(global_config.bbfile);
        free(global_config.bbfile);
        global_config.bbfile = absolute_bbfile;

        if (daemonize() < 0) {
            free_allocated_memory();
            exit(EXIT_FAILURE);
        }

        pidfd = create_pid_file(pidfile);
        if (pidfd < 0) {
            free_allocated_memory();
            exit(EXIT_FAILURE);
        }
    }

    while (!global_terminate_server) {
        global_restart_server = 0;

        initialize_server();
        if (global_restart_server)
            printf("SIGHUP signal handled. reconfiguring server\n\n");

        configure_server(config_path);
    }

    if (!global_rconfig.fground)
        release_pid_file(pidfile, pidfd);

    printf("termination signal handled. terminating server\n");

    free(config_path);

    return 0;
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

void free_allocated_memory()
{
    free(global_config.bbfile);
    free_peers(global_rconfig.peer, global_rconfig.peer_count);
}

void configure_server(char *config_path)
{
    global_config = (ServerConfig){.thmax = 25, .thincr = 5, .bbport = 9000, .fdebug = false, .bbfile = NULL};
    global_rconfig = (ReplicationConfig){.fground = 0, .pdebug = 0, .rport = 9001, .peer = NULL};

    read_config_file(&global_config, &global_rconfig, config_path);

    if (!global_config.bbfile) {
        printf("BBFILE required\n");
        exit(EXIT_FAILURE);
    }

    if (global_config.bbport == global_rconfig.rport) {
        printf("bbport and rport must be different\n");
        exit(EXIT_FAILURE);
    }

    print_config();

    bb_init();
}

char *resolve_path(char *path)
{
    char absolute_path[256];
    if (!realpath(path, absolute_path)) {
        perror("Failed to resolve relative path as absolute");
        exit(EXIT_FAILURE);
    }

    return strdup(absolute_path);
}
