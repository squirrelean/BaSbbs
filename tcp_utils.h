#ifndef TCP_UTILS_H
#define TCP_UTILS_H
#include <stddef.h>

int create_listen_socket(int port, int backlog);
int read_line(const int fd, char *buffer, const size_t maxlen);

#endif
