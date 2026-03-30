#ifndef GLOBALS_H
#define GLOBALS_H
#include "read_config.h"
#include <signal.h>

extern ServerConfig global_config;
extern ReplicationConfig global_rconfig;
extern volatile sig_atomic_t global_restart_server;
extern volatile sig_atomic_t global_terminate_server;

#endif
