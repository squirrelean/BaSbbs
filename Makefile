CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = bbserv
SRCS = main.c read_config.c
DEPS = read_config.h

$(TARGET): $(SRCS) $(DEPS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm $(TARGET)
