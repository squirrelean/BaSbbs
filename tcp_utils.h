#ifndef TCP_UTILS_H
#define TCP_UTILS_H
#include <stddef.h>

int create_listen_socket(int port, int backlog);
int read_line(const int fd, char *buffer, const size_t maxlen);
int get_connection_info(const char *rhost, const int rport, struct sockaddr_in *sin);
int connect_to_server(struct sockaddr_in *sin, int *sd);
char *read_from_server(int sd, int timeout);

#endif
