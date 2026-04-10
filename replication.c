#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bbfile.h"
#include "globals.h"
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

    long write_offset = get_bbfile_offset();

    while (read_line(peer_sd, buffer, sizeof(buffer)) > 0) {
        if (!strncmp(buffer, "PRECOM SYN", 10))
            send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);

        else if (!strncmp(buffer, "ABRT", 4) || !strncmp(buffer, "OK", 2))
            break;

        else if (!strncmp(buffer, "COM", 3)) {
            sscanf(buffer, "COM %s %ld %64s %ld %4096[^\n]", operation, &next_id, username, &msg_num,
                   message);

            global_next_id = next_id;

            int op_status = -1;
            if (!strncmp(operation, "WRITE", 4))
                op_status = bb_write(username, message);

            else if (!strncmp(operation, "REPLACE", 8) && msg_num > 0)
                op_status = bb_replace(username, msg_num, message);

            if (op_status < 0)
                send(peer_sd, "NAK\n", sizeof("NAK\n"), 0);
            else
                send(peer_sd, "ACK\n", sizeof("ACK\n"), 0);
        }

        else if (!strncmp(buffer, "NOK", 3)) {
            // truncate(global_config.bbfile, write_offset);
            // decrement the bbfile id
        }
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
    char buffer[1024];
    for (int i = 0; i < global_rconfig.peer_count; i++) {
        int bytes_read = read_line(sd[i], buffer, sizeof(buffer));
        if (bytes_read <= 0)
            return -1;

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
