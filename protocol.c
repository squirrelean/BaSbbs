#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bbfile.h"
#include "tcp_utils.h"

void send_welcome_msg(int client_fd);
void quit_command(int client_fd);
void user_command(int client_fd, char *buffer, char *username, int user_len);
void write_command(int client_fd, char *buffer, char *username);
void read_command(int client_fd, char *buffer);
void replace_command(int client_fd, char *buffer, char *current_user);

void handle_client(int client_fd)
{
    char buffer[4096];
    char current_user[64] = "anonymous coward";

    send_welcome_msg(client_fd);

    while (1) {
        int bytes_read = read_line(client_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
            break;

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
    printf("user_command: username: %s\n", username);

    snprintf(msg, sizeof(msg), "1.0 HELLO %s greetings\n", username);
    write(client_fd, msg, strlen(msg));
}

void write_command(int client_fd, char *buffer, char *username)
{
    char msg[1024];
    long message_number;

    if (strlen(buffer) < 6)
        snprintf(msg, sizeof(msg), "Usage: WRITE message\n");
    else if ((message_number = bb_write(username, buffer + 6)) != -1) {
        snprintf(msg, sizeof(msg), "3.0 WROTE %ld\n", message_number);
    } else {
        snprintf(msg, sizeof(msg), "3.2 ERROR WRITE failed to handle file\n");
    }

    write(client_fd, msg, strlen(msg));
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

    if (strlen(buffer) < 8)
        snprintf(msg, sizeof(msg), "Usage: REPLACE message-number/message\n");
    else {
        char *endptr;
        long message_number = strtol(buffer + 8, &endptr, 10);

        int x = bb_replace(current_user, message_number, endptr + 1);
        if (x == -1 || x == -3)
            snprintf(msg, sizeof(msg), "4.2 ERROR REPLACE failed to handle file\n");
        else if (x == -2)
            snprintf(msg, sizeof(msg), "4.1 UNKNOWN %ld\n", message_number);
        else
            snprintf(msg, sizeof(msg), "4.0 REPLACED %ld\n", message_number);
    }
    write(client_fd, msg, strlen(msg));
}
