DRIVER_NAME := rbtree_test

HOME_PATH = $(HOME)
obj-m += $(DRIVER_NAME).o
KERNEL_SRC:= $(HOME_PATH)/repository/linux/linux-5.10.y-raspi

PWD := $(shell pwd)
ARCH = arm64
CROSS_COMPILE = aarch64-none-linux-gnu-

# 是否编译测试代码
TESTS_DIR := $(PWD)/tests
ifeq ($(wildcard $(TESTS_DIR)),)
	TESTS_BUILD_CMD :=
	TESTS_CLEAN_CMD :=
	TESTS_DEPLOY_CMD :=
else
	TESTS_BUILD_CMD := $(MAKE) -C $(TESTS_DIR) all
	TESTS_CLEAN_CMD := $(MAKE) -C $(TESTS_DIR) clean
	TESTS_DEPLOY_CMD := cp $(TESTS_DIR)/*.o $(HOME_PATH)/repository/linux/buildroot-2025.11/output/target/root/
endif


build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(TESTS_BUILD_CMD)

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules clean
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order .tmp* .cache
	$(TESTS_CLEAN_CMD)


.PHONY: all deploy qemu
deploy:
	cp $(DRIVER_NAME).ko $(HOME_PATH)/repository/linux/buildroot-2025.11/output/target/root/
	$(TESTS_DEPLOY_CMD)
	find $(HOME_PATH)/repository/linux/buildroot-2025.11/output/target/root/ -name "*.o" -exec chmod a+x {} +
	make -C $(HOME_PATH)/repository/linux/buildroot-2025.11/ -j$(nproc)
	cp $(HOME_PATH)/repository/linux/buildroot-2025.11/output/images/rootfs.ext4 $(HOME_PATH)/tftp/

qemu: build deploy
	qemu-system-aarch64 \
		-M raspi4b \
		-cpu cortex-a72 \
		-m 2G \
		-nographic \
		-kernel $(HOME_PATH)/tftp/raspi4b/Image \
		-dtb $(HOME_PATH)/tftp/raspi4b/bcm2711-rpi-4-b.dtb \
		-sd $(HOME_PATH)/tftp/rootfs.ext4 \
		-append "console=ttyAMA0 earlycon=pl011,0xfe201000 root=/dev/mmcblk1 rw rootwait"
