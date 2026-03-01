#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

struct ft5x06_drv_data {
        struct gpio_desc *reset_gpio;
        struct i2c_client *ft5x06_client;
        struct input_dev *ft5x06_input_dev;
        struct work_struct ft5x06_irq_work;
        
};

int ft5x06_read_reg(struct i2c_client *client, u8 reg_addr)
{
        u8 data;
        // 定义两个 i2c_msg 结构体，分别表示写操作和读操作
        struct i2c_msg msgs[2] = {
                [0] = {
                        .addr = client->addr, // 想要读的设备地址
                        .flags = 0, // 写操作
                        .len = sizeof(reg_addr),
                        .buf = &reg_addr, // 写入想要读取的寄存器地址
                },
                [1] = {
                        .addr = client->addr,
                        .flags = I2C_M_RD, // 读操作
                        .len = sizeof(data),
                        .buf = &data,
                },
        };

        // 使用 i2c_transfer 函数进行 i2c 总线读取操作
        if (i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs)) != ARRAY_SIZE(msgs))
                return -EIO;

        return data;
}

int ft5x06_write_reg(struct i2c_client *client, u8 reg_addr, u8 data)
{
        u8 buf[2] = {
                reg_addr,
                data,
        };

        struct i2c_msg msgs[1] = {
                [0] = {
                        .addr = client->addr, // 想要写入的设备地址
                        .flags = 0, // 写操作
                        .len = ARRAY_SIZE(buf),
                        .buf = buf, // 写入想要读取的寄存器地址
                },
        };

        if (i2c_transfer(client->adapter, msgs, 1) != ARRAY_SIZE(msgs))
                return -EIO;

        return 0;
}

irqreturn_t ft5x06_threaded_fn(int irq, void *dev_id)
{
        int TOUCH1_XH, TOUCH1_XL, x;
        int TOUCH1_YH, TOUCH1_YL, y;
        int TD_STATUS;

        struct ft5x06_drv_data *drv_data = (struct ft5x06_drv_data *)dev_id;
        struct i2c_client *client = drv_data->ft5x06_client;
        struct input_dev *input_dev = drv_data->ft5x06_input_dev;

        // 从寄存器中读取触摸坐标数据
        TOUCH1_XH = ft5x06_read_reg(client, 0x03);
        TOUCH1_XL = ft5x06_read_reg(client, 0x04);

        x = ((TOUCH1_XH << 8) | TOUCH1_XL) & 0xfff;

        TOUCH1_YH = ft5x06_read_reg(client, 0x05);
        TOUCH1_YL = ft5x06_read_reg(client, 0x06);
        y = ((TOUCH1_YH << 8) | TOUCH1_YL) & 0xfff;

        // 读取触摸状态寄存器
        TD_STATUS = ft5x06_read_reg(client, 0x02);
        TD_STATUS = TD_STATUS & 0xf;

        if (TD_STATUS == 0) {
                // 触摸释放
                input_report_key(input_dev, BTN_TOUCH, 0);
                input_sync(input_dev);
        } else {
                // 触摸按下
                input_report_key(input_dev, BTN_TOUCH, 1);
                input_report_abs(input_dev, ABS_X, x);
                input_report_abs(input_dev, ABS_Y, y);
                input_sync(input_dev);
        }

        return IRQ_HANDLED;
}

irqreturn_t ft5x06_irq_handler(int irq, void *dev_id)
{
        struct ft5x06_drv_data *drv_data = (struct ft5x06_drv_data *)dev_id;
        schedule_work(&drv_data->ft5x06_irq_work);
        return IRQ_WAKE_THREAD;
}

int ft5x06_init(struct gpio_desc *reset_gpio)
{
        int ret;
        // 设置 reset GPIO 为输出,并拉低 5ms 后拉高
        // 这是一个复位操作,用于初始化 ft5x06 设备
        ret = gpiod_direction_output(reset_gpio, 0);
        if (ret < 0)
                return ret;
        msleep(5);
        ret = gpiod_direction_output(reset_gpio, 1);
        if (ret < 0) {
                return ret;
        }
        return 0;
}

int ft5x06_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        int ret;

        struct ft5x06_drv_data *drv_data;
        struct device *dev;
        struct input_dev *input_dev;

        dev = &client->dev;
        drv_data = devm_kzalloc(dev, sizeof(struct ft5x06_drv_data), GFP_KERNEL);
        if (!drv_data)
                return -ENOMEM;

        i2c_set_clientdata(client, drv_data);

        // 保存 i2c_client
        drv_data->ft5x06_client = client;

        // 获取 reset gpio 描述符
        drv_data->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
        if (IS_ERR(drv_data->reset_gpio))
                return PTR_ERR(drv_data->reset_gpio);
        if (!drv_data->reset_gpio)
                return -ENODEV;

        ret = devm_request_threaded_irq(dev, client->irq, ft5x06_irq_handler, ft5x06_threaded_fn,
                                        IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "ft5x06 irq",
                                        drv_data);
        if (ret < 0)
                return -ENODEV;

        // 分配一个 input 设备
        input_dev = devm_input_allocate_device(dev);
        drv_data->ft5x06_input_dev = input_dev;

        input_dev->name = "ft5x06_dev";
        set_bit(EV_KEY, input_dev->evbit);
        set_bit(BTN_TOUCH, input_dev->keybit);
        set_bit(EV_ABS, input_dev->evbit);
        set_bit(ABS_X, input_dev->absbit);
        set_bit(ABS_Y, input_dev->absbit);

        // 设置 input 设备的绝对坐标范围
        input_set_abs_params(input_dev, ABS_X, 0, 800, 0, 0);
        input_set_abs_params(input_dev, ABS_Y, 0, 1280, 0, 0);

        ret = input_register_device(input_dev);
        if (ret < 0) {
                input_free_device(input_dev);
                return ret;
        }
        
        // ft5x06 复位初始化
        ret = ft5x06_init(drv_data->reset_gpio);
        if (ret < 0)
                return ret;

        return 0;
}

int ft5x06_remove(struct i2c_client *client)
{
        // struct ft5x06_drv_data *drv_data = i2c_get_clientdata(client);

        return 0;
}

struct i2c_device_id ft5x06_match_table[] = { { .name = "ft5x06" }, {} };
MODULE_DEVICE_TABLE(i2c, ft5x06_match_table);

static const struct of_device_id ft5x06_of_match_table[] = { { .compatible = "even629,ft5x06" },
                                                             {} };
MODULE_DEVICE_TABLE(of, ft5x06_of_match_table);

struct i2c_driver ft5x06_drver = {
        .driver = {
                .name = "ft5x06",
                .owner = THIS_MODULE,
                .of_match_table = ft5x06_of_match_table,
        },
        .probe = ft5x06_probe,
        .remove = ft5x06_remove,
        .id_table = ft5x06_match_table,
};

module_i2c_driver(ft5x06_drver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629 <asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is test sample for ft5x06");
