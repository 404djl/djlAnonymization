INCLUDE_PATH = /usr/local/include/opencv4
LOG_DIR = log

CC = g++

CFLAGS = -g -fPIC -Wall -I$(INCLUDE_PATH)

SRCS = $(wildcard *.c) $(wildcard $(LOG_DIR)/*.c)

OBJS = $(patsubst %.c, %.o, $(SRCS))
TARGET = ./test/lib/libAnonymization.so

.PHONY: target clean

%.o: %.c
	$(CC) -w -c $< $(CFLAGS) -std=c99 -o $@

target: $(OBJS)
	$(CC) -shared -o $(TARGET) $(OBJS)
	rm -f $(OBJS)

clean:
	rm -f $(OBJS) $(TARGET)