CC      := aarch64-none-linux-gnu-gcc
SYSROOT := /home/zhaohang/tools/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc
CFLAGS  := -g -Wall --sysroot=$(SYSROOT)
LDFLAGS := -static -lgpiod

SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:%.c=%.o)
PWD     ?= $(shell pwd)


all: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJS) .cache compile_commands.json
deploy:
	cp $(PWD)/*.o ~/share
.PHONY: all clean
