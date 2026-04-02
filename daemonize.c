#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int daemonize()
{
    pid_t bgpid = fork();
    if (bgpid < 0) {
        perror("daemonization fork");
        return -1;
    }

    if (bgpid)
        exit(EXIT_SUCCESS);

    // Detach from controlling tty
    int ttyfd = open("/dev/tty", O_RDWR);
    if (ttyfd < 0) {
        perror("daemonize: failed to open tty fd");
        return -1;
    }
    if (ioctl(ttyfd, TIOCNOTTY, 0) < 0)
        perror("daemonize: ioctl failed to detach from tty");
    close(ttyfd);

    if (mkdir("run", 0755) < 0)
        perror("daemonize: failed to create run directory");
    if (chdir("run") < 0)
        perror("daemonize: failed to change to safe directory");

    umask(0);

    // Close all descriptors
    for (int i = 0; i < getdtablesize(); i++)
        close(i);

    // Redirect stdin to bit bucket
    int nullfd = open("/dev/null", O_RDWR);
    if (nullfd != 0) {
        dup2(nullfd, 0);
        close(nullfd);
    }

    // Redirect stdout and stderr to log file
    int logfd = open("bbserv.log", O_WRONLY | O_CREAT | O_APPEND);
    if (logfd != 1) {
        dup2(logfd, 1);
        close(logfd);
    }
    dup2(1, 2);

    return 0;
}

int create_pid_file(const char *filename)
{
    int pidfd = open(filename, O_RDWR | O_CREAT, 0640);
    if (pidfd < 0) {
        perror("daemonize: failed to handle opening of pidfile");
        return -1;
    }

    if (lockf(pidfd, F_TLOCK, 0) < 0) {
        perror("daemonize: cannot lock pid file");
        close(pidfd);
        return -1;
    }

    char buff[32];
    int n = snprintf(buff, sizeof(buff), "%d\n", getpid());
    if (write(pidfd, buff, n) < 0) {
        perror("daemonize: failed to write pid to pid file");
        return -1;
    }

    return pidfd;
}

void release_pid_file(const char *filename, const int pidfd)
{
    if (lockf(pidfd, F_ULOCK, 0) < 0)
        perror("daemonize: failed to unlock pid file");

    if (unlink(filename) < 0)
        perror("daemonize: failed to delete pid file");

    close(pidfd);
}
