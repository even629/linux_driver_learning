DRIVER_NAME := copy_to_user

obj-m += $(DRIVER_NAME).o
KERNEL_SRC:=/home/zhaohang/repository/linux/linux-5.10.y-raspi

PWD ?=$(shell pwd)
ARCH = arm64
CROSS_COMPILE = aarch64-none-linux-gnu-


all: build

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(MAKE) -C $(PWD)/tests

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules clean
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order .tmp* .cache
	$(MAKE) -C $(PWD)/tests clean

.PHONY: all deploy qemu
deploy:
	cp $(PWD)/tests/*.o /home/zhaohang/repository/linux/busybox-1.37.0/_install
	cp $(DRIVER_NAME).ko /home/zhaohang/repository/linux/busybox-1.37.0/_install
	/bin/bash /home/zhaohang/repository/linux/busybox-1.37.0/deploy.sh

qemu: deploy
	qemu-system-aarch64 \
		-M raspi4b \
		-cpu cortex-a72 \
		-m 2G \
		-nographic \
		-kernel /home/zhaohang/tftp/raspi4b/Image \
		-dtb /home/zhaohang/tftp/raspi4b/bcm2711-rpi-4-b.dtb \
		-initrd /home/zhaohang/tftp/initramfs.cpio.gz \
		-append "console=ttyAMA0 earlycon=pl011,0xfe201000 rdinit=/linuxrc"


