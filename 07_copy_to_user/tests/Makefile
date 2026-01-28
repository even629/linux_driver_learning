CC = aarch64-none-linux-gnu-gcc
CFLAGS = -g -Wall

all: test_copy_to_user.o

test_copy_to_user.o: test_copy_to_user.c
	$(CC) $< $(CFLAGS) -o $@
clean:
	rm -rf test_copy_to_user.o compile_commands.json .cache
