#include <signal.h>
#include <stddef.h>

#include "globals.h"

volatile sig_atomic_t global_restart_server = 0;
volatile sig_atomic_t global_terminate_server = 0;

void handle_signal(int signal);

void initialize_signals()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

void handle_signal(int signal)
{
    if (signal == SIGHUP)
        global_restart_server = 1;
    else if (signal == SIGQUIT || signal == SIGINT)
        global_terminate_server = 1;
}
