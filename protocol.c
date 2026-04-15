#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bbfile.h"
#include "globals.h"
#include "lock.h"
#include "replication.h"
#include "tcp_utils.h"

void send_welcome_msg(int client_fd);
void quit_command(int client_fd);
void user_command(int client_fd, char *buffer, char *username, int user_len);
void write_command(int client_fd, char *buffer, char *username);
void read_command(int client_fd, char *buffer);
void replace_command(int client_fd, char *buffer, char *current_user);
void write_to_client(int client_fd, char *msg);

void handle_client(int client_fd)
{
    char buffer[4096];
    char current_user[64] = "anonymous coward";

    send_welcome_msg(client_fd);

    while (!global_restart_server && !global_terminate_server) {
        int bytes_read = read_line(client_fd, buffer, sizeof(buffer));
        if (bytes_read < 0)
            break;
        else if (!bytes_read)
            continue;

        if (!strncmp(buffer, "QUIT", 4)) {
            quit_command(client_fd);
            break;
        } else if (!strncmp(buffer, "USER", 4)) {
            user_command(client_fd, buffer, current_user, sizeof(current_user));
        } else if (!strncmp(buffer, "WRITE", 5)) {
            write_command(client_fd, buffer, current_user);
        } else if (!strncmp(buffer, "READ", 4)) {
            read_command(client_fd, buffer);
        } else if (!strncmp(buffer, "REPLACE", 7)) {
            replace_command(client_fd, buffer, current_user);
        } else {
            char *msg = "Unknown command\n";
            write(client_fd, msg, strlen(msg));
        }
    }
}

void send_welcome_msg(int client_fd)
{
    const char *message = "0.0 WELCOME ver 1.0: USER READ WRITE REPLACE QUIT spoken here\n";
    write(client_fd, message, strlen(message));
}

void quit_command(int client_fd)
{
    const char *msg = "9.0 BYE Goodbye\n";
    write(client_fd, msg, strlen(msg));
}

void user_command(int client_fd, char *buffer, char *username, int user_len)
{
    char msg[128];
    if (strlen(buffer) < 5) {
        snprintf(msg, sizeof(msg), "Usage: USER name\n");
        write(client_fd, msg, strlen(msg));
        return;
    }

    char *entered_name = buffer + 5;
    if (strchr(entered_name, '/')) {
        snprintf(msg, sizeof(msg), "1.2 BAD %s Invalid username\n", entered_name);
        write(client_fd, msg, strlen(msg));
        return;
    }

    strncpy(username, entered_name, user_len - 1);
    username[user_len - 1] = '\0';

    snprintf(msg, sizeof(msg), "1.0 HELLO %s greetings\n", username);
    write(client_fd, msg, strlen(msg));
}

void write_command(int client_fd, char *buffer, char *username)
{
    char msg[1024];
    BbMeta write_meta = {.backup = NULL, .status = -1};

    if (strlen(buffer) < 6) {
        snprintf(msg, sizeof(msg), "Usage: WRITE message\n");
        write_to_client(client_fd, msg);
        return;
    }

    if (!global_rconfig.peer_count) {
        write_lock();

        write_meta = bb_write(username, buffer + 6);

        if (write_meta.status == 0)
            snprintf(msg, sizeof(msg), "3.0 WROTE %ld\n", write_meta.previous_id);
        else
            snprintf(msg, sizeof(msg), "3.3 ERROR WRITE failed to handle file\n");

        write_unlock();

        write_to_client(client_fd, msg);
        return;
    }

    int socks[global_rconfig.peer_count];
    memset(socks, -1, sizeof(socks));

    int replica_status;

    char pmsg[5120];
    snprintf(pmsg, sizeof(pmsg), "COM %s|%ld|%s|%d|%s\n", "WRITE", global_next_id, username, -1, buffer + 6);

    if ((replica_status = replica_master_init(socks, pmsg)) == 0) {
        write_lock();
        write_meta = bb_write(username, buffer + 6);
        write_unlock();
    }

    if (replica_status == -1 || write_meta.status < 0)
        snprintf(msg, sizeof(msg), "3.3 ERROR WRITE failure during PRECOMMIT phase\n");

    // Broadcast unsuccessful message to peers if master failed write or any peer sent NAK.
    if (replica_status == -2 || write_meta.status < 0) {
        broadcast_to_peers(socks, "NOK WRITE\n");
    }

    // Broadcast successful message to peers
    else if (write_meta.status == 0 && replica_status == 0) {
        snprintf(msg, sizeof(msg), "3.0 WROTE %ld\n", write_meta.previous_id);
        broadcast_to_peers(socks, "OK\n");
    }

    close_socks(socks);

    write_to_client(client_fd, msg);

    free(write_meta.backup);
}

void read_command(int client_fd, char *buffer)
{
    char msg[2048];
    long message_number;

    if (strlen(buffer) < 5) {
        snprintf(msg, sizeof(msg), "Usage: READ message-number\n");
    } else {
        char *endptr;
        message_number = strtol(buffer + 5, &endptr, 10);
        char *message_to_read = NULL;
        int rv = bb_read(message_number, &message_to_read);
        switch (rv) {
        case -1:
            snprintf(msg, sizeof(msg), "2.2 ERROR READ failed to handle file\n");
            break;
        case -2:
            snprintf(msg, sizeof(msg), "2.1 UNKNOWN %ld Could not find message on file\n", message_number);
            break;
        default:
            snprintf(msg, sizeof(msg), "2.0 MESSAGE %ld %s", message_number, message_to_read);
        }
        free(message_to_read);
    }

    write(client_fd, msg, strlen(msg));
}

void replace_command(int client_fd, char *buffer, char *current_user)
{
    char msg[2048];

    if (strlen(buffer) < 8) {
        snprintf(msg, sizeof(msg), "Usage: REPLACE message-number/message\n");
        write(client_fd, msg, strlen(msg));
        return;
    }

    char *endptr;
    long message_number = strtol(buffer + 8, &endptr, 10);
    BbMeta r_meta = {.backup = NULL, .status = -10};

    int replica_status = -10;

    if (!global_rconfig.peer_count) {
        write_lock();
        r_meta = bb_replace(current_user, message_number, endptr + 1);
        write_unlock();

        if (r_meta.status == 0) {
            snprintf(msg, sizeof(msg), "4.0 REPLACED %ld\n", message_number);
            delete_backup(r_meta);
        }

        else if (r_meta.status == -1)
            snprintf(msg, sizeof(msg), "4.2 ERROR REPLACE failed to handle file\n");

        else if (r_meta.status == -2)
            snprintf(msg, sizeof(msg), "4.1 UNKNOWN %ld\n", message_number);

        write_to_client(client_fd, msg);
        free(r_meta.backup);
        return;
    }

    int socks[global_rconfig.peer_count];
    memset(socks, -1, sizeof(socks));

    char pmsg[5120];

    snprintf(pmsg, sizeof(pmsg), "COM %s|%ld|%s|%ld|%s\n", "REPLACE", global_next_id, current_user,
             message_number, endptr + 1);

    if ((replica_status = replica_master_init(socks, pmsg)) == 0) {
        write_lock();
        r_meta = bb_replace(current_user, message_number, endptr + 1);
        write_unlock();
    }

    if (r_meta.status == 0) {
        snprintf(msg, sizeof(msg), "4.0 REPLACED %ld\n", message_number);
        broadcast_to_peers(socks, "OK\n");
        delete_backup(r_meta);
    }

    else if (replica_status == -1) {
        snprintf(msg, sizeof(msg), "4.3 ERROR REPLACE failed to synchronize with peers\n");
        broadcast_to_peers(socks, "ABRT\n");
    }

    else {
        if (replica_status == -2)
            snprintf(msg, sizeof(msg), "4.1 ERROR REPLACE COMMIT phase failed\n");

        else if (r_meta.status == -1)
            snprintf(msg, sizeof(msg), "4.2 ERROR REPLACE failed to handle file\n");
        else if (r_meta.status == -2)
            snprintf(msg, sizeof(msg), "4.1 UNKNOWN %ld\n", message_number);

        broadcast_to_peers(socks, "NOK REPLACE\n");
    }

    close_socks(socks);

    write_to_client(client_fd, msg);

    free(r_meta.backup);
}

void write_to_client(int client_fd, char *msg)
{
    write(client_fd, msg, strlen(msg));
    if (global_rconfig.pdebug)
        printf("master-client: %s\n", msg);
}
