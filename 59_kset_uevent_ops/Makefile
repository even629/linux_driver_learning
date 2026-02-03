DRIVER_NAME := kset_uevent_ops_test

obj-m += $(DRIVER_NAME).o
# KERNEL_SRC:=/home/zhaohang/repository/linux/linux-5.10.y-raspi 
KERNEL_SRC:=/home/zhaohang/repository/linux/rk3568_linux_5.10/kernel
PWD ?=$(shell pwd)
ARCH = arm64
# CROSS_COMPILE = aarch64-none-linux-gnu-
CROSS_COMPILE=/home/zhaohang/repository/linux/rk3568_linux_5.10/prebuilts/gcc/linux-x86/aarch64/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-

# 是否编译测试代码
TESTS_DIR := $(PWD)/tests
ifeq ($(wildcard $(TESTS_DIR)),)
	TESTS_BUILD_CMD :=
	TESTS_CLEAN_CMD :=
	TESTS_DEPLOY_CMD :=
else
	TESTS_BUILD_CMD := $(MAKE) -C $(TESTS_DIR) all
	TESTS_CLEAN_CMD := $(MAKE) -C $(TESTS_DIR) clean
	TESTS_DEPLOY_CMD := cp $(TESTS_DIR)/*.o /home/zhaohang/share/
endif


all: build

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules
	$(TESTS_BUILD_CMD)

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(PWD) modules clean
	rm -rf *.ko *.o *.mod.o *.mod.c *.symvers *.order .tmp* .cache
	$(TESTS_CLEAN_CMD)


.PHONY: all deploy
deploy:
	cp $(DRIVER_NAME).ko /home/zhaohang/share/
	$(TESTS_DEPLOY_CMD)	

