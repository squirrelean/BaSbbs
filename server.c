#include "globals.h"
#include "protocol.h"
#include "read_config.h"
#include "tcp_utils.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

void create_threads(int master_sock);
void *monitor_thread();

struct monitor_t {
    pthread_mutex_t mutex;
    int total_threads;
    int active_threads;
    int idle_t_to_reap;
};

struct monitor_t mon = {PTHREAD_MUTEX_INITIALIZER, 0, 0, 0};

void initialize_server()
{
    int master_sock = create_listen_socket(global_config.bbport, 32);
    if (master_sock == -1)
        return;

    // Makes the master socket non blocking on accept.
    int flags = fcntl(master_sock, F_GETFL, 0);
    fcntl(master_sock, F_SETFL, flags | O_NONBLOCK);

    printf("Server listening on port: %d\n", global_config.bbport);

    // Preallocate Tincr threads on startup
    pthread_mutex_lock(&mon.mutex);
    create_threads(master_sock);
    pthread_mutex_unlock(&mon.mutex);

    monitor_thread();

    // wait for all threads to terminate before restarting
    while (true) {
        pthread_mutex_lock(&mon.mutex);
        if (mon.total_threads == 0) {
            pthread_mutex_unlock(&mon.mutex);
            break;
        }
        pthread_mutex_unlock(&mon.mutex);

        sleep(1);
    }

    close(master_sock);

    mon.active_threads = 0;
    mon.idle_t_to_reap = 0;
    mon.total_threads = 0;

    return;
}

void *run_client(void *arg)
{
    int master_sock = (int)(long)(arg);
    int slave_sock;

    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);

    struct pollfd pol;

    while (!global_terminate_server && !global_restart_server) {
        pthread_mutex_lock(&mon.mutex);
        if (mon.idle_t_to_reap > 0 && !global_terminate_server) {
            // mon.total_threads--;
            mon.idle_t_to_reap--;
            pthread_mutex_unlock(&mon.mutex);
            break;
        }
        pthread_mutex_unlock(&mon.mutex);

        // Prevent master_sock from blocking on accept.
        pol.fd = master_sock;
        pol.events = POLLIN;
        int timeout = poll(&pol, 1, 2000);
        if (timeout <= 0)
            continue;

        slave_sock = accept(master_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (slave_sock < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            perror("slave accept failure");
            continue;
        }

        pthread_mutex_lock(&mon.mutex);
        mon.active_threads++;
        if (mon.active_threads == mon.total_threads && mon.total_threads <= global_config.thmax)
            create_threads(master_sock);
        pthread_mutex_unlock(&mon.mutex);

        handle_client(slave_sock);
        close(slave_sock);

        pthread_mutex_lock(&mon.mutex);
        mon.active_threads--;
        pthread_mutex_unlock(&mon.mutex);
    }

    pthread_mutex_lock(&mon.mutex);
    mon.total_threads--;
    pthread_mutex_unlock(&mon.mutex);

    return NULL;
}

void *monitor_thread()
{
    const int wakeup_interval = 20;

    while (!global_restart_server && !global_terminate_server) {
        sleep(wakeup_interval);

        pthread_mutex_lock(&mon.mutex);
        if (mon.total_threads > global_config.thincr &&
            mon.active_threads < (mon.total_threads - global_config.thincr - 1)) {
            mon.idle_t_to_reap = global_config.thincr;
        }
        pthread_mutex_unlock(&mon.mutex);
    }

    return NULL;
}

void create_threads(int master_sock)
{
    pthread_t t_id;
    pthread_attr_t t_attr;
    pthread_attr_init(&t_attr);
    pthread_attr_setdetachstate(&t_attr, PTHREAD_CREATE_DETACHED);

    for (int i = 0; i < global_config.thincr; i++) {
        if (pthread_create(&t_id, &t_attr, run_client, (void *)(long)master_sock) == 0)
            mon.total_threads++;
    }
}
