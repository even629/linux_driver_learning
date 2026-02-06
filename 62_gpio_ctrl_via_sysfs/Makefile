CC      := aarch64-none-linux-gnu-gcc
CFLAGS  := -g -Wall

SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:%.c=%.o)
PWD     ?= $(shell pwd)

all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJS) .cache compile_commands.json
deploy:
	cp $(PWD)/*.o ~/share
.PHONY: all clean
