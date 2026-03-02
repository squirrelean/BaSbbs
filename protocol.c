#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "tcp_utils.h"

void send_welcome_msg(int client_fd);
void quit_command(int client_fd);
void user_command(int client_fd, char *buffer, char *username, int user_len);

void handle_client(int client_fd)
{
    char buffer[1024];
    char current_user[64] = "anonymous coward";

    send_welcome_msg(client_fd);

    while (1) {
        int bytes_read = read_line(client_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0)
            break;

        if (!strncmp(buffer, "QUIT", 4)) {
            quit_command(client_fd);
            break;
        }
        else if (!strncmp(buffer, "USER", 4) && bytes_read >= 5) {
            user_command(client_fd, buffer, current_user, sizeof(current_user));
        }
        else {
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
    char *entered_name = buffer + 5;
    if (strchr(entered_name, '/')) {
        char msg[128];
        snprintf(msg, sizeof(msg), "1.2 BAD %s Invalid username\n", entered_name);
        write(client_fd, msg, strlen(msg));
        return;
    }

    strncpy(username, entered_name, user_len - 1);
    username[user_len - 1] = '\0';

    char msg[128];
    snprintf(msg, sizeof(msg), "1.0 HELLO %s greetings\n", username);
    write(client_fd, msg, strlen(msg));
}
