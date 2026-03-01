#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/**
 * @brief 从 I2C 设备的寄存器中读取数据
 * @param fd 打开的 I2C 设备文件描述符
 * @param slave_addr I2C 设备的从机地址
 * @param reg_addr 要读取的寄存器地址
 * @return 寄存器的值
 */
int ft5x06_read_reg(int fd, unsigned char slave_addr, unsigned char reg_addr)
{
        unsigned char data;        
        int ret;
        // 定义两个 i2c_msg 结构体, 第一个用于写入寄存器地址, 第二个用于读取数据
        struct i2c_msg dev_msgs[] = {
                [0] = {
                        .addr = slave_addr,
                        .flags = 0,
                        .len = sizeof(reg_addr),
                        .buf = &reg_addr,
                },
                [1] = {
                        .addr = slave_addr,
                        .flags = I2C_M_RD,
                        .len = sizeof(data),
                        .buf = &data,
                }
        };
        struct i2c_rdwr_ioctl_data i2c_msgs = {
                .msgs = dev_msgs,
                .nmsgs = 2  
        };
        

        ret = ioctl(fd, I2C_RDWR, &i2c_msgs);
        if(ret < 0){
                printf("read error\n");
                return ret;
        }

        return data;
}

int ft5x06_write_reg(int fd, unsigned char slave_addr, unsigned char reg_addr, unsigned char data)
{
        int ret = 0;
        unsigned char buf[2] = {reg_addr, data};
        
        struct i2c_msg dev_msgs[1] = {
                [0] = {
                        .addr = slave_addr,
                        .flags = 0,
                        .len = 2,
                        .buf = buf,                        
                },
        };

        struct i2c_rdwr_ioctl_data i2c_msgs = {
                .msgs = dev_msgs,
                .nmsgs = 1,
        };

        ret = ioctl(fd, I2C_RDWR, &i2c_msgs);
        if(ret < 0)
                printf("write error\n");                

        return ret;
}

int main(int argc, char **argv)
{
        int fd;
        int ID_G_THGROUP;

        // 打开 I2C 设备文件
        fd = open("/dev/i2c-1", O_RDWR);
        if (fd < 0) {
                printf("open error\n");
                return fd;
        }

        unsigned char data = 0x55;
        // 向 0x38 地址的寄存器 0x80 写入 0x55
        ft5x06_write_reg(fd, 0x38, 0x80, data);
        // 从 0x38 地址的寄存器 0x80 读取数据
        ID_G_THGROUP = ft5x06_read_reg(fd, 0x38, 0x80);

        printf("ID_G_THGROUP is 0x%02X\n", ID_G_THGROUP);

        close(fd);
        return 0;
}
