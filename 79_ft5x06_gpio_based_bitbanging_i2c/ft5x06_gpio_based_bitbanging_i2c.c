#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>

struct ft5x06_drv_data {
        struct i2c_client *ft5x06_client;
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


int ft5x06_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
        
        struct ft5x06_drv_data *drv_data;
        struct device *dev = &client->dev;
        
        
        drv_data = devm_kzalloc(dev, sizeof(struct ft5x06_drv_data), GFP_KERNEL);
        if (!drv_data)
                return -ENOMEM;

        i2c_set_clientdata(client, drv_data);

        // 保存 i2c_client
        drv_data->ft5x06_client = client;        
        
        return 0;
}

int ft5x06_remove(struct i2c_client *client)
{
        // struct ft5x06_drv_data *drv_data = i2c_get_clientdata(client);

        return 0;
}

struct i2c_device_id ft5x06_match_table[] = { { .name = "my-ft5x06" }, {} };
MODULE_DEVICE_TABLE(i2c, ft5x06_match_table);

static const struct of_device_id ft5x06_of_match_table[] = { { .compatible = "my-ft5x06" },
                                                             {} };
MODULE_DEVICE_TABLE(of, ft5x06_of_match_table);

struct i2c_driver ft5x06_drver = {
        .driver = {
                .name = "my-ft5x06",
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
