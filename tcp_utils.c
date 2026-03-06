#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int create_listen_socket(int port, int backlog)
{
    struct sockaddr_in sin;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failure");
        return -1;
    }

    // Prevent address already in use if execution occurs too quickly
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("bind failure");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        perror("listen failure");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int read_line(const int fd, char *buffer, const size_t maxlen)
{
    size_t i = 0;
    char ch;
    size_t n;

    while (i < maxlen - 1) {
        n = recv(fd, &ch, 1, 0);
        if (n <= 0)
            return n;

        if (ch == '\r')
            continue;

        if (ch == '\n')
            break;

        buffer[i++] = ch;
    }
    buffer[i] = '\0';

    return i;
}
