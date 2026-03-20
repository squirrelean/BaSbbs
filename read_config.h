#ifndef READ_CONFIG_H
#define READ_CONFIG_H

typedef struct {
    int thmax;
    int thincr;
    int bbport;
    int fdebug;
    char *bbfile;
} ServerConfig;

typedef struct {
    char *host;
    int port;
} EndPoint;

typedef struct {
    int fground;
    int pdebug;
    int rport;
    EndPoint *peer;
    int peer_count;
} ReplicationConfig;

void read_config_file(ServerConfig *config, ReplicationConfig *rconfig, char *config_path);
void free_peers(EndPoint *peers, int peer_count);

#endif
