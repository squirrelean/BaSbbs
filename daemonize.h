#ifndef DAEMONIZE_H
#define DAEMONIZE_H

int daemonize();
int create_pid_file(const char *filename);
void release_pid_file(const char *filename, const int pidfd);

#endif
