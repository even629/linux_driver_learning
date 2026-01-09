CC=/home/zhaohang/repository/linux/rk3568_linux_5.10/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
CFLAGS = -g -Wall

all: test_led.o

test_led.o: test_led.c
	$(CC) $< $(CFLAGS) -o $@
clean:
	rm -rf test_led.o compile_commands.json .cache
