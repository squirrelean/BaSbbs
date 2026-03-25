#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_config.h"
bool parse_bool_flag(char *value);
void create_peer(EndPoint *peer, char *value);
void append_peer(EndPoint **peers, EndPoint peer, int *size, int index);
void free_peers(EndPoint *peers, int peer_count);
char *parse_path(char *value);

void read_config_file(ServerConfig *config, ReplicationConfig *rconfig, char *config_path)
{
    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        perror("Could not open file");
        return;
    }

    int peer_size = 10;
    int peer_index = 0;
    if (rconfig->peer)
        free_peers(rconfig->peer, rconfig->peer_count);
    EndPoint *peers = malloc(peer_size * sizeof(EndPoint));
    rconfig->peer_count = 0;

    char buffer[256], key[256], value[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "%255s %255s", key, value) == 2) {
            if (!strcmp(key, "THMAX"))
                config->thmax = atoi(value);
            else if (!strcmp(key, "THINCR"))
                config->thincr = atoi(value);
            else if (!strcmp(key, "BBPORT"))
                config->bbport = atoi(value);
            else if (!strcmp(key, "FDEBUG"))
                config->fdebug = parse_bool_flag(value);
            else if (!strcmp(key, "BBFILE")) {
                if (config->bbfile) {
                    free(config->bbfile);
                    config->bbfile = NULL;
                }
                config->bbfile = parse_path(value);
            } else if (!strcmp(key, "FOREGROUND"))
                rconfig->fground = parse_bool_flag(value);
            else if (!strcmp(key, "PDEBUG"))
                rconfig->pdebug = parse_bool_flag(value);
            else if (!strcmp(key, "RPORT"))
                rconfig->rport = atoi(value);
            else if (!strcmp(key, "PEER") && peers) {
                EndPoint peer;
                create_peer(&peer, value);
                append_peer(&peers, peer, &peer_size, peer_index++);
                rconfig->peer_count++;
            }
        }
    }
    fclose(fp);

    if (!rconfig->peer_count) {
        free(peers);
        peers = NULL;
    }

    rconfig->peer = peers;
}

bool parse_bool_flag(char *value)
{
    if (!strcmp(value, "true") || atoi(value) == 1)
        return true;

    return false;
}

char *parse_path(char *value)
{
    char path[256];

    char *ptr = realpath(value, path);
    if (!ptr) {
        perror("failed to handle bbfile absolue path");
        return NULL;
    }

    return strdup(path);
}

void create_peer(EndPoint *peer, char *value)
{
    char *sep = strchr(value, ':');
    if (sep) {
        *sep = '\0';
        peer->host = strdup(value);
        peer->port = atoi(sep + 1);
    }
}

void append_peer(EndPoint **peers, EndPoint peer, int *size, int index)
{
    if (index >= *size) {
        int new_cap = (*size) * 2;
        EndPoint *temp = realloc(*peers, new_cap * sizeof(EndPoint));
        if (!temp)
            return;

        *peers = temp;
        *size = new_cap;
    }
    (*peers)[index] = peer;
}

void free_peers(EndPoint *peers, int peer_count)
{
    for (int i = 0; i < peer_count; i++) {
        free(peers[i].host);
    }
    free(peers);
}
