DRIVER_NAME := llseek_test

obj-m += $(DRIVER_NAME).o
KERNEL_SRC:=/home/zhaohang/repository/linux/linux-5.10.y-raspi

PWD ?=$(shell pwd)
ARCH = arm64
CROSS_COMPILE = aarch64-linux-gnu-

# 是否编译测试代码
TESTS_DIR := $(PWD)/tests
ifeq ($(wildcard $(TESTS_DIR)),)
	TESTS_BUILD_CMD :=
	TESTS_CLEAN_CMD :=
	TESTS_DEPLOY_CMD :=
else
	TESTS_BUILD_CMD := $(MAKE) -C $(TESTS_DIR) all
	TESTS_CLEAN_CMD := $(MAKE) -C $(TESTS_DIR) clean
	TESTS_DEPLOY_CMD := cp $(TESTS_DIR)/*.o /home/zhaohang/repository/linux/busybox-1.37.0/_install
endif


all: build

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(TESTS_BUILD_CMD)

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules clean
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order .tmp* .cache
	$(TESTS_CLEAN_CMD)


.PHONY: all deploy qemu
deploy:
	cp $(DRIVER_NAME).ko /home/zhaohang/repository/linux/busybox-1.37.0/_install
	$(TESTS_DEPLOY_CMD)
	chmod a+x /home/zhaohang/repository/linux/busybox-1.37.0/_install/*.o
	/bin/bash /home/zhaohang/repository/linux/busybox-1.37.0/deploy.sh

qemu: build deploy
	qemu-system-aarch64 \
		-M raspi4b \
		-cpu cortex-a72 \
		-m 2G \
		-nographic \
		-kernel /home/zhaohang/tftp/raspi4b/Image \
		-dtb /home/zhaohang/tftp/raspi4b/bcm2711-rpi-4-b.dtb \
		-initrd /home/zhaohang/tftp/initramfs.cpio.gz \
		-append "console=ttyAMA0 earlycon=pl011,0xfe201000 rdinit=/linuxrc"


