#ifndef REPLICATION_H
#define REPLICATION_H

int replica_master_init(int socks[], const char *pcol_msg);
void broadcast_to_peers(const int sd[], const char *message);
void close_socks(int sd[]);

#endif
