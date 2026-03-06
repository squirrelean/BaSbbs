CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = bbserv
SRCS = main.c read_config.c server.c tcp_utils.c protocol.c bbfile.c lock.c
DEPS = read_config.h server.h  tcp_utils.h protocol.h bbfile.h globals.h lock.h

$(TARGET): $(SRCS) $(DEPS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm $(TARGET)
