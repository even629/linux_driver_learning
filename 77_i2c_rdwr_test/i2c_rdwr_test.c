#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

/*
 * 从 I2C 设备读取寄存器值
 * @param fd: I2C 设备文件句柄
 * @param reg_addr: 要读取的寄存器地址
 */
int ft5x06_read_reg(int fd, unsigned char reg_addr)
{
        unsigned char data = 0;

        write(fd, &reg_addr, 1);
        read(fd, &data, 1);

        printf("reg value is %x\n", data);
        return data;
}

/*
 * 向 I2C 设备写入寄存器值
 * @param fd: I2C 设备文件句柄
 * @param reg_addr: 要写入的寄存器地址
 * @param data: 要写入的数据
 */
void ft5x06_write_reg(int fd, unsigned char reg_addr, unsigned char data)
{
        unsigned char wr_data[2] = { reg_addr, data };
        write(fd, wr_data, 2);
}

int main(int argc, char **argv)
{
        int fd;

        fd = open("/dev/i2c-1", O_RDWR);
        if (fd < 0) {
                printf("open error\n");
                return fd;
        }

        // 设置从设备地址为 0x38
        ioctl(fd, I2C_SLAVE_FORCE, 0x38);

        // 向寄存器 0x80 写入数据 0x66
        ft5x06_write_reg(fd, 0x80, 0x66);

        // 读取寄存器 0x80 的值
        ft5x06_read_reg(fd, 0x80);

        return 0;
}
