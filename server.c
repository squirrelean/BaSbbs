#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>

#include "read_config.h"
#include "tcp_utils.h"
#include "protocol.h"

int initialize_server(ServerConfig *config)
{
    int master_sock, slave_sock;

    master_sock = create_listen_socket(config->bbport, 35);
    if (master_sock == -1)
        return -1;
    printf("Server listening on port: %d\n", config->bbport);

    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);

    while (true) {
        slave_sock = accept(master_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (slave_sock < 0) {
            perror("slave accept failure");
            continue;
        }

        // handle the client
        handle_client(slave_sock);
    }

}
