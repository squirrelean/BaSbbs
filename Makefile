CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

TARGET = rbbserv
SRCS = main.c read_config.c server.c tcp_utils.c protocol.c bbfile.c lock.c daemonize.c
DEPS = read_config.h server.h  tcp_utils.h protocol.h bbfile.h globals.h lock.h daemonize.h

$(TARGET): $(SRCS) $(DEPS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) -pthread

clean:
	rm $(TARGET)
