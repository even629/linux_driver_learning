#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

// 定义 I2C 总线的时钟线和数据线对应的 GPIO 引脚编号
#define I2C_SCL 11
#define I2C_SDA 12

// 声明两个 GPIO 描述符变量,用于保存 SCL 和 SDA 引脚的描述符
struct gpio_desc *i2c_scl_desc;
struct gpio_desc *i2c_sda_desc;

// I2C 起始条件函数
void i2c_start(void)
{
        // 将 SCL 和 SDA 引脚设置为输出模式,并初始化为高电平
        // 这是 I2C 总线的空闲状态
        gpiod_direction_output(i2c_scl_desc, 1);
        gpiod_direction_output(i2c_sda_desc, 1);
        mdelay(1); // 延时 1 毫秒

        // 将 SDA 引脚设置为低电平,保持 SCL 为高电平
        // 这将产生 I2C 总线的起始条件
        gpiod_direction_output(i2c_sda_desc, 0);
        mdelay(1); // 延时 1 毫秒

        // 将 SCL 引脚设置为低电平
        // 起始条件建立完成
        gpiod_direction_output(i2c_scl_desc, 0);
        mdelay(1); // 延时 1 毫秒
}

// I2C 停止条件函数
void i2c_stop(void)
{
        // 将 SCL 和 SDA 引脚设置为低电平
        gpiod_direction_output(i2c_scl_desc, 0);
        gpiod_direction_output(i2c_sda_desc, 0);
        mdelay(1); // 延时 1 毫秒

        // 将 SCL 引脚设置为高电平
        gpiod_direction_output(i2c_scl_desc, 1);
        mdelay(1); // 延时 1 毫秒

        // 将 SDA 引脚设置为高电平
        // 这将产生 I2C 总线的停止条件
        gpiod_direction_output(i2c_sda_desc, 1);
        mdelay(1); // 延时 1 毫秒
}

// 发送ACK信号
void i2c_send_ack(int ack)
{
        // 设置SDA线为输出模式
        gpiod_direction_output(i2c_sda_desc, 0);

        if (ack) {
                // 发送ACK信号, SDA线拉低
                gpiod_direction_output(i2c_sda_desc, 0);
        } else {
                // 发送NACK信号, SDA线拉高
                gpiod_direction_output(i2c_sda_desc, 1);
        }

        // 拉高SCL线1ms,然后拉低
        gpiod_direction_output(i2c_scl_desc, 1);
        mdelay(1);
        gpiod_direction_output(i2c_scl_desc, 0);
}

// 接收ACK信号
int i2c_recv_ack(void)
{
        int value = 0;

        // 设置SDA线为输入模式
        gpiod_direction_input(i2c_sda_desc);

        // 拉高SCL线1ms
        gpiod_direction_output(i2c_scl_desc, 1);
        mdelay(1);

        // 读取SDA线的电平状态
        if (gpiod_get_value(i2c_sda_desc)) {
                value = 1; // 接收到NACK信号
        } else {
                value = 0; // 接收到ACK信号
        }

        // 拉低SCL线
        gpiod_direction_output(i2c_scl_desc, 0);

        // 设置SDA线为输出模式并拉高
        gpiod_direction_output(i2c_sda_desc, 1);

        return value;
}

void i2c_send_data(int data)
{
        int i;
        int value;

        // 设置SCL线为输出模式并拉低
        gpiod_direction_output(i2c_scl_desc, 0);

        // 发送8位数据
        for (i = 0; i < 8; i++) {
                // 获取当前位的值
                value = (data << i) & 0x80;

                // 根据当前位的值设置SDA线
                if (value) {
                        gpiod_direction_output(i2c_sda_desc, 1);
                } else {
                        gpiod_direction_output(i2c_sda_desc, 0);
                }

                // 拉高SCL线1ms,然后拉低
                gpiod_direction_output(i2c_scl_desc, 1);
                mdelay(1);
                gpiod_direction_output(i2c_scl_desc, 0);
                mdelay(1);
        }
}

int i2c_recv_data(void)
{
        int i;
        int temp = 0;
        int data = 0;

        // 设置SDA线为输入模式
        gpiod_direction_input(i2c_sda_desc);
        mdelay(1);

        // 接收8位数据
        for (i = 0; i < 8; i++) {
                // 拉低SCL线1ms
                gpiod_direction_output(i2c_scl_desc, 0);
                mdelay(1);

                // 拉高SCL线1ms
                gpiod_direction_output(i2c_scl_desc, 1);
                mdelay(1);

                // 读取SDA线的电平状态
                data = gpiod_get_value(i2c_sda_desc);

                // 根据当前位的值更新接收数据
                if (data) {
                        temp = (temp << 1) | data;
                } else {
                        temp = (temp << 1) & ~data;
                }
        }

        // 拉低SCL线
        gpiod_direction_output(i2c_scl_desc, 0);
        mdelay(1);

        // 设置SDA线为输出模式并拉高
        gpiod_direction_output(i2c_sda_desc, 1);

        return temp;
}

// ft5x06 触摸屏写寄存器函数
void ft5x06_write_reg(int addr, int reg, int value)
{
        int ack;

        // 开始 I2C 通信
        i2c_start();

        // 发送触摸屏设备地址(写操作)
        i2c_send_data(addr << 1 | 0x00);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send write + addr error\n");
                goto end;
        }

        // 发送寄存器地址
        i2c_send_data(reg);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send reg error\n");
                goto end;
        }

        // 发送要写入的值
        i2c_send_data(value);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send value error\n");
        }

end:
        // 结束 I2C 通信
        i2c_stop();
}

//  ft5x06 触摸屏读寄存器函数
int ft5x06_read_reg(int addr, int reg)
{
        int ack = 0;
        int data = 0;

        // 开始 I2C 通信
        i2c_start();

        // 发送触摸屏设备地址(写操作)
        i2c_send_data(addr << 1 | 0x00);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send write + addr error\n");
                goto end;
        }

        // 发送要读取的寄存器地址
        i2c_send_data(reg);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send reg error\n");
                goto end;
        }

        // 重新开始 I2C 通信,发送读操作地址
        i2c_start();
        i2c_send_data(addr << 1 | 0x01);
        ack = i2c_recv_ack();
        if (ack) {
                printk("send read + addr error\n");
                goto end;
        }

        // 读取寄存器值
        data = i2c_recv_data();
        printk("data is %d\n", data);

        // 发送 ACK 以结束读操作
        i2c_send_ack(0);

end:
        // 结束 I2C 通信
        i2c_stop();

        return data;
}

static int __init ft5x06_soft_i2c_init(void)
{
        // 将 GPIO 编号转换为 GPIO 描述符
        i2c_scl_desc = gpio_to_desc(I2C_SCL);
        if (i2c_scl_desc == NULL) {
                printk("gpio_to_desc error for SCL pin\n");
                return -1;
        }

        i2c_sda_desc = gpio_to_desc(I2C_SDA);
        if (i2c_sda_desc == NULL) {
                printk("gpio_to_desc error for SDA pin\n");
                return -1;
        }

        // 将 GPIO 引脚设置为输出模式,并初始化为高电平
        // 这是 I2C 总线的空闲状态
        gpiod_direction_output(i2c_scl_desc, 1);
        gpiod_direction_output(i2c_sda_desc, 1);

        ft5x06_write_reg(0x38, 0x80, 0x33);
        ft5x06_read_reg(0x38, 0x80);

        return 0;
}

static void __exit ft5x06_soft_i2c_exit(void)
{
        // 释放 GPIO 描述符
        gpiod_put(i2c_scl_desc);
        gpiod_put(i2c_sda_desc);
}

module_init(ft5x06_soft_i2c_init);
module_exit(ft5x06_soft_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629 <asqwgo@outlook.com>");
MODULE_DESCRIPTION("ft5x06 software i2c emulator");
