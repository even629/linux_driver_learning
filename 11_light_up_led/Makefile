DRIVER_NAME := light_up_led

obj-m += $(DRIVER_NAME).o
# KERNEL_SRC:=/home/zhaohang/repository/linux/linux-5.10.y-raspi
KERNEL_SRC:=/home/zhaohang/repository/linux/rk3568_linux_5.10/kernel
PWD ?=$(shell pwd)
ARCH = arm64
# CROSS_COMPILE = aarch64-none-linux-gnu-
CROSS_COMPILE=/home/zhaohang/repository/linux/rk3568_linux_5.10/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

all: build

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(MAKE) -C $(PWD)/tests all

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules clean
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order .tmp* .cache
	$(MAKE) -C $(PWD)/tests clean

.PHONY: all deploy

deploy:
	cp $(DRIVER_NAME).ko /home/zhaohang/share
	cp tests/*.o /home/zhaohang/share
