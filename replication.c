#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bbfile.h"
#include "globals.h"
#include "lock.h"
#include "tcp_utils.h"

int connect_to_peers(struct sockaddr_in sin[], int socks[]);
int phase_init(const int sd[], const char *bmsg, const char *pos_msg, const char *neg_msg);
void broadcast_to_peers(const int sd[], const char *message);
int await_response(const int sd[], const char *status);
void close_socks(int sd[]);

int replica_master_init(int socks[], const char *pcol_msg)
{
    struct sockaddr_in peers[global_rconfig.peer_count];

    if (connect_to_peers(peers, socks) < 0)
        return -1;

    // Precommit phase
    if (phase_init(socks, "PRECOM SYN\n", "ACK\n", "ABRT\n") < 0) {
        return -1;
    }

    // Commit phase
    if (phase_init(socks, pcol_msg, "ACK\n", "NAK\n") < 0) {
        return -1;
    }

    return 0;
}

int phase_init(const int sd[], const char *bmsg, const char *pos_msg, const char *neg_msg)
{
    broadcast_to_peers(sd, bmsg);
    if (await_response(sd, pos_msg) < 0) {
        broadcast_to_peers(sd, neg_msg);
        return -1;
    }
    return 0;
}

void replica_slave_init(int peer_sd)
{
    char buffer[5120];
    long next_id = -1;
    long msg_num = -1;
    char username[128];
    char operation[32];
    char message[5120];

    BbMeta op_meta = {.backup = NULL};

    while (!global_terminate_server && !global_restart_server) {
        int bytes_read = read_line(peer_sd, buffer, sizeof(buffer));
        if (bytes_read < 0)
            break;

        if (!strncmp(buffer, "PRECOM SYN", 10)) {
            send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);
            continue;
        }

        else if (!strncmp(buffer, "ABRT", 4))
            break;

        // Master begins commit phase
        if (!strncmp(buffer, "COM", 3)) {
            write_lock();

            sscanf(buffer, "COM %s %ld %64s %ld %4096[^\n]", operation, &next_id, username, &msg_num,
                   message);

            global_next_id = next_id;

            if (!strncmp(operation, "WRITE", 4)) {
                op_meta = bb_write(username, message);
            }

            else if (!strncmp(operation, "REPLACE", 8) && msg_num > 0)
                op_meta = bb_replace(username, msg_num, message);

            if (op_meta.status < 0)
                send(peer_sd, "NAK\n", sizeof("NAK\n"), 0);
            else
                send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);

            // Get second protocol message from master.
            bytes_read = read_line(peer_sd, buffer, sizeof(buffer));

            if (bytes_read > 0 && !strncmp(buffer, "NOK WRITE", 9)) {
                // Rollback write operation
                bb_rollback(op_meta);
            }

            else if (bytes_read > 0 && !strncmp(buffer, "NOK REPLACE", 11)) {
                // Rollback replace operation
                bb_rollback(op_meta);
            }

            else if (!strncmp(buffer, "OK", 2))
                delete_backup(op_meta);

            write_unlock();
        }
    }

    close(peer_sd);

    free(op_meta.backup);
}

void *replica_listener(void *arg)
{
    (void)arg;

    struct sockaddr_in peer_addr;
    unsigned int peer_addr_len = sizeof(peer_addr);

    int peer_sd;
    int peer_sock = create_listen_socket(global_rconfig.rport, 32);

    // Makes the master socket non blocking on accept.
    int flags = fcntl(peer_sock, F_GETFL, 0);
    fcntl(peer_sock, F_SETFL, flags | O_NONBLOCK);

    struct pollfd pol;

    while (!global_terminate_server && !global_restart_server) {
        // Prevent master_sock from blocking on accept.
        pol.fd = peer_sock;
        pol.events = POLLIN;
        int timeout = poll(&pol, 1, 2000);
        if (timeout <= 0)
            continue;

        peer_sd = accept(peer_sock, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (peer_sd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            perror("replication: accept failure");
            continue;
        }

        replica_slave_init(peer_sd);
    }

    close(peer_sock);

    return NULL;
}

int connect_to_peers(struct sockaddr_in sin[], int socks[])
{
    for (int i = 0; i < global_rconfig.peer_count; i++) {
        char *peer_host = global_rconfig.peer[i].host;
        int peer_port = global_rconfig.peer[i].port;
        if (get_connection_info(peer_host, peer_port, sin) < 0)
            return -1;

        if (connect_to_server(sin, &socks[i]) < 0)
            return -1;
    }

    return 0;
}

void broadcast_to_peers(const int sd[], const char *message)
{
    if (global_rconfig.pdebug)
        printf("master broadcasting: %s\n", message);

    for (int i = 0; i < global_rconfig.peer_count; i++) {
        send(sd[i], message, strlen(message), 0);
    }
}

int await_response(const int sd[], const char *status)
{
    char buffer[1024];
    for (int i = 0; i < global_rconfig.peer_count; i++) {
        int bytes_read = read_line(sd[i], buffer, sizeof(buffer));
        if (bytes_read <= 0)
            return -1;

        if (global_rconfig.pdebug)
            printf("peer %d replied with: %s\n", i, buffer);

        if (strncmp(buffer, status, strlen(status)) != 0)
            return -1;
    }

    return 0;
}

void close_socks(int sd[])
{
    for (int i = 0; i < global_rconfig.peer_count; i++)
        close(sd[i]);
}
