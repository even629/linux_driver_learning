#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define GPIO_REG_BASE 0xFDD60000
#define GPIO_SWPORT_DDR_L_OFFSET 0x0008
#define GPIO_SWPORT_DR_L_OFFSET 0x0000
#define SIZE_MAP 0x1000

void led_on(unsigned char *base)
{
        // 设置LED灯的方向为输出
        *(volatile unsigned int *)(base + GPIO_SWPORT_DDR_L_OFFSET) = 0x80008044;
        // 将LED灯打开
        *(volatile unsigned int *)(base + GPIO_SWPORT_DR_L_OFFSET) = 0x80008040;
}

void led_off(unsigned char *base)
{
        // 设置LED灯的方向为输出
        *(volatile unsigned int *)(base + GPIO_SWPORT_DDR_L_OFFSET) = 0x80008044;
        // 将LED灯关闭
        *(volatile unsigned int *)(base + GPIO_SWPORT_DR_L_OFFSET) = 0x80000040;
}

int main(int argc, char *argv[])
{
        int fd;
        unsigned char *map_base;

        fd = open("/dev/mem", O_RDWR | O_NDELAY);
        if (fd < 0) {
                perror("/dev/mem open error");
                exit(EXIT_FAILURE);
        }

        // 将物理地址映射到用户空间
        map_base = mmap(NULL, SIZE_MAP, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_REG_BASE);
        if (map_base == MAP_FAILED) {
                perror("mmap error\n");
                close(fd);
                exit(EXIT_FAILURE);
        }

        while (1) {
                led_on(map_base); // 打开 LED 灯
                sleep(1);      // 等待 1 s
                led_off(map_base);// 关闭 LED 灯
                sleep(1);      // 等待 1 s
        }
        // 解除映射
        munmap(map_base, SIZE_MAP);

        close(fd);

        return 0;
}
