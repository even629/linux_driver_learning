CC      := aarch64-linux-gnu-gcc
CFLAGS  := -g -Wall

SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:%.c=%.o)

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJS) .cache compile_commands.json

.PHONY: all clean
