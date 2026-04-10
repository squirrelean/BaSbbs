#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bbfile.h"
#include "globals.h"
#include "tcp_utils.h"

int connect_to_peers(struct sockaddr_in sin[], int socks[]);
void broadcast_to_peers(const int sd[], const char *message);
int await_response(const int sd[], const char *status);
void close_socks(int sd[]);

int replica_master_init(long msg_num, const char *username, const char *msg, char op)
{
    struct sockaddr_in peers[global_rconfig.peer_count];
    int socks[global_rconfig.peer_count];
    memset(socks, -1, sizeof(socks));

    if (connect_to_peers(peers, socks) < 0)
        return -1;

    // Precommit phase
    broadcast_to_peers(socks, "PRECOM SYN\n");
    if (await_response(socks, "ACK\n") < 0) {
        broadcast_to_peers(socks, "ABRT\n");
        close_socks(socks);
        return -1;
    }

    // Commit phase
    char buffer[5120];
    snprintf(buffer, sizeof(buffer), "COM %ld %ld %s %s\n", global_next_id, msg_num, username, msg);

    broadcast_to_peers(socks, buffer);
    if (await_response(socks, "ACK\n") < 0) {
        broadcast_to_peers(socks, "NAK\n");
        close_socks(socks);
        return -1;
    }

    long op_status = -1;
    switch (op) {
    case 'w':
        op_status = bb_write(username, msg + 6);
        break;
    case 'r':
        op_status = bb_replace(username, msg_num, msg + 8);
        break;
    }

    if (op_status < 0) {
        broadcast_to_peers(socks, "NOK\n");
        close_socks(socks);
        return -1;
    }

    broadcast_to_peers(socks, "OK\n");

    close_socks(socks);

    return 0;
}

void replica_slave_init(int peer_sd)
{
    char buffer[5120];
    long next_id = -1;
    long msg_num = -1;
    char username[65];
    char message[4097];

    while (read_line(peer_sd, buffer, sizeof(buffer)) > 0) {
        if (!strncmp(buffer, "PRECOM SYN", sizeof("PRECOM SYN")))
            send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);

        else if (!strncmp(buffer, "ABRT", sizeof("ABRT")) || !strncmp(buffer, "OK", sizeof("OK")))
            break;

        else if (!strncmp(buffer, "COM", sizeof("COM"))) {
            sscanf(buffer, "COM %ld %ld %64s %4096s", &next_id, &msg_num, username, message);
            if (next_id < 0) {
                send(peer_sd, "NAK\n", sizeof("NAK\n"), 0);
                break;
            }

            global_next_id = next_id;

            int op_status = -1;
            if (!strncmp(message, "WRITE", sizeof("WRITE")))
                op_status = bb_write(username, message + 6);

            else if (!strncmp(message, "REPLACE", sizeof("REPLACE")) && msg_num > 0)
                op_status = bb_replace(username, msg_num, message + 8);

            if (op_status < 0)
                send(peer_sd, "NAK\n", sizeof("NAK\n"), 0);
            else
                send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);
        }

        // else if (!strncmp(buffer, "NOK\n", sizeof("NOK\n")))
        //  rollback WRITE/REPLACE
    }
    close(peer_sd);
}

void *replica_listener(void *arg)
{
    struct sockaddr_in peer_addr;
    unsigned int peer_addr_len = sizeof(peer_addr);

    int peer_sd;
    int peer_sock = create_listen_socket(global_rconfig.rport, 32);

    while (!global_terminate_server && !global_restart_server) {
        peer_sd = accept(peer_sock, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (peer_sd >= 0)
            replica_slave_init(peer_sd);
    }

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
    for (int i = 0; i < global_rconfig.peer_count; i++) {
        send(sd[i], message, strlen(message), 0);
    }
}

int await_response(const int sd[], const char *status)
{
    const int timeout = 2000;
    char *peer_response = NULL;

    for (int i = 0; i < global_rconfig.peer_count; i++) {
        peer_response = read_from_server(sd[i], timeout);
        if (!peer_response)
            return -1;

        int match = strcmp(peer_response, status);

        free(peer_response);

        if (match != 0)
            return -1;
    }

    free(peer_response);

    return 0;
}

void close_socks(int sd[])
{
    for (int i = 0; i < global_rconfig.peer_count; i++)
        close(sd[i]);
}
