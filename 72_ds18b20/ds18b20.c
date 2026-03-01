#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

// ROM Command
#define ROM_CMD_SEARCH_ROM 0xF0
#define ROM_CMD_READ_ROM 0x33
#define ROM_CMD_MATCH_ROM 0x55
#define ROM_CMD_SKIP_ROM 0xCC
#define ROM_CMD_ALARM_SEARCH 0xEC

// Function Command
#define FUNC_CMD_CONVERT_T 0x44
#define FUNC_CMD_WRITE_SCRATCHPAD 0x4E
#define FUNC_CMD_READ_SCRATCHPAD 0xBE
#define FUNC_CMD_COPY_SCRATCHPAD 0x48
#define FUNC_CMD_RECALL_E2 0xB8
#define FUNC_CMD_READ_POWER_SUPPLY 0xB4

struct ds18b20 {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        struct gpio_desc *gpiod;
        struct fasync_struct *fa;
        u8 precision;
        // wait_queue_head_t wq;
};

int ds18b20_reset(struct gpio_desc *gpiod)
{
        int timeout;

        // host send reset pulse
        gpiod_direction_output(gpiod, 0);
        udelay(500); // >= 480 us

        // host release the bus
        gpiod_direction_input(gpiod);
        udelay(60); // 15 us ~ 60 us

        // ds18b20 send presence pulse
        timeout = 240;
        while (gpiod_get_value(gpiod)) { // 60 us ~ 240 us
                if (timeout == 0)
                        goto err;
                udelay(1);
                timeout--;
        }

        // resistor pullup
        timeout = 240;
        while (!gpiod_get_value(gpiod)) { // 60 us ~ 240 us
                if (timeout == 0)
                        goto err;
                udelay(1);
                timeout--;
        }

        udelay(480);

        return 0;
err:
        return -EIO;
}

void ds18b20_write_bit(struct gpio_desc *gpiod, char bit)
{
        gpiod_direction_output(gpiod, 0);

        if (bit) {
                udelay(6);
                gpiod_direction_input(gpiod);
                udelay(80); // 60 ~ 120
        } else {
                udelay(86);
                gpiod_direction_input(gpiod);
        }

        udelay(1);
}

void ds18b20_write_byte(struct gpio_desc *gpiod, char byte)
{
        int i;
        for (i = 0; i < 8; i++)
                ds18b20_write_bit(gpiod, (byte >> i) & 0x1);
}

u8 ds18b20_read_bit(struct gpio_desc *gpiod)
{
        char bit;

        gpiod_direction_output(gpiod, 0);
        udelay(2); // 1 us ~ 15 us, as smaller as possible

        gpiod_direction_input(gpiod);
        udelay(12); // wait for 12 us then read value

        bit = gpiod_get_value(gpiod);
        udelay(50); // >= 45us

        return bit;
}

u8 ds18b20_read_byte(struct gpio_desc *gpiod)
{
        int i;
        char value = 0;
        for (i = 0; i < 8; i++)
                value |= ds18b20_read_bit(gpiod) << i;

        return value;
}

int ds18b20_open(struct inode *inode, struct file *file)
{
        struct ds18b20 *drv_data = container_of(inode->i_cdev, struct ds18b20, cdev);
        //struct gpio_desc *gpiod = drv_data->gpiod;

        file->private_data = drv_data;
        // 精度设置为12
        drv_data->precision = 12;

        pr_info("%s success, default precision is %d\n", __func__, drv_data->precision);

        return 0;
}

int ds18b20_release(struct inode *inode, struct file *file)
{
        pr_info("%s\n", __func__);
        return 0;
}

ssize_t ds18b20_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        char ls_byte, ms_byte;
        struct ds18b20 *drv_data = file->private_data;
        struct gpio_desc *gpiod = drv_data->gpiod;
        int16_t raw_temp;
        size_t count = min(sizeof(int16_t), size);
        int ret;

        // 复位
        ret = ds18b20_reset(gpiod);
        if (ret < 0) {
                pr_err("%s: reset fail\n", __func__);
                return ret;
        }
        // 只有一个ds18b20, 发送Skip ROM
        ds18b20_write_byte(gpiod, ROM_CMD_SKIP_ROM);
        // Function Command, 发送Convert T
        ds18b20_write_byte(gpiod, FUNC_CMD_CONVERT_T);

        switch (drv_data->precision) {
        case 12:
                msleep(750);
                break;
        case 11:
                msleep(375);
                break;
        case 10:
                msleep(188);
                break;
        case 9:
                msleep(94);
                break;
        default:
                pr_err("unsupported precision\n");
                return -EFAULT;
        }

        // 复位
        ret = ds18b20_reset(gpiod);
        if (ret < 0) {
                pr_err("%s: reset fail\n", __func__);
                return ret;
        }
        // 只有一个ds18b20, 发送Skip ROM
        ds18b20_write_byte(gpiod, ROM_CMD_SKIP_ROM);
        // Function Command, 发送Read Scratchpad
        ds18b20_write_byte(gpiod, FUNC_CMD_READ_SCRATCHPAD);
        // read val
        ls_byte = ds18b20_read_byte(gpiod);
        ms_byte = ds18b20_read_byte(gpiod);

        raw_temp = (int16_t)(((u16)ms_byte << 8) | ls_byte);

        pr_info("raw_temp is %d\n", raw_temp);

        if (copy_to_user(buf, &raw_temp, count) != 0)
                return -EFAULT;

        // TODO CRC

        return count;
}

#define DS_CMD_START _IO('D', 0)
#define DS_CMD_SET_PREC _IOW('D', 1, int)
#define DS_CMD_GET_TEMP _IOR('D', 2, int)

long ds18b20_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        char ls_byte, ms_byte;
        struct ds18b20 *drv_data = file->private_data;
        struct gpio_desc *gpiod = drv_data->gpiod;
        int16_t raw_temp;
        int ret;

        switch (cmd) {
        case DS_CMD_START:
                // 复位
                ret = ds18b20_reset(gpiod);
                if (ret < 0) {
                        pr_err("%s: ds18b20 reset failed\n", __func__);
                        return ret;
                }
                // 只有一个ds18b20, 发送Skip ROM
                ds18b20_write_byte(gpiod, ROM_CMD_SKIP_ROM);
                // Function Command, 发送Convert T
                ds18b20_write_byte(gpiod, FUNC_CMD_CONVERT_T);

                switch (drv_data->precision) {
                case 12:
                        msleep(750);
                        break;
                case 11:
                        msleep(375);
                        break;
                case 10:
                        msleep(188);
                        break;
                case 9:
                        msleep(94);
                        break;
                default:
                        pr_err("unsupported precision\n");
                        return -EFAULT;
                }
                break;
        case DS_CMD_SET_PREC:
                break;
        case DS_CMD_GET_TEMP:
                // 复位
                ret = ds18b20_reset(gpiod);
                if (ret < 0) {
                        pr_err("%s: reset fail\n", __func__);
                        return ret;
                }
                // 只有一个ds18b20, 发送Skip ROM
                ds18b20_write_byte(gpiod, ROM_CMD_SKIP_ROM);
                // Function Command, 发送Read Scratchpad
                ds18b20_write_byte(gpiod, FUNC_CMD_READ_SCRATCHPAD);
                // read val
                ls_byte = ds18b20_read_byte(gpiod);
                ms_byte = ds18b20_read_byte(gpiod);

                raw_temp = (int16_t)(((u16)ms_byte << 8) | ls_byte);

                pr_info("raw_temp is %d\n", raw_temp);

                // if (copy_to_user((void *)arg, &raw_temp, sizeof(int16_t)) != 0)
                //         return -EFAULT;
                if(put_user(raw_temp, (int16_t __user *)arg))
                        return -EFAULT;
                break;
        default:
                pr_err("unknown ioctl cmd\n");
                return -EFAULT;
        }

        return 0;
}

int ds18b20_fasync(int fd, struct file *file, int on)
{
        struct ds18b20 *drv_data = file->private_data;
        return fasync_helper(fd, file, on, &drv_data->fa);
}

struct file_operations ds18b20_fops = {
        .owner = THIS_MODULE,
        .open = ds18b20_open,
        .release = ds18b20_release,
        .read = ds18b20_read,
        .llseek = no_llseek,
        .unlocked_ioctl = ds18b20_unlocked_ioctl,
};

static int ds18b20_probe(struct platform_device *pdev)
{
        int ret;
        //struct device *dev = &pdev->dev;
        struct ds18b20 *ds18b20;

        ds18b20 = kzalloc(sizeof(*ds18b20), GFP_KERNEL);
        if (!ds18b20) {
                ret = -ENOMEM;
                goto err_kzalloc;
        }

        ret = alloc_chrdev_region(&ds18b20->dev_num, 0, 1, "ds18b20");
        if (ret < 0)
                goto err_chrdev_region;

        cdev_init(&ds18b20->cdev, &ds18b20_fops);
        ds18b20->cdev.owner = THIS_MODULE;
        ret = cdev_add(&ds18b20->cdev, ds18b20->dev_num, 1);
        if (ret < 0)
                goto err_cdev;

        ds18b20->class = class_create(THIS_MODULE, "one-wire");
        if (IS_ERR(ds18b20->class)) {
                ret = PTR_ERR(ds18b20->class);
                goto err_class;
        }

        // /sys/class/one-wire/ds18b20
        ds18b20->dev = device_create(ds18b20->class, NULL, ds18b20->dev_num, NULL, "ds18b20");
        if (IS_ERR(ds18b20->dev)) {
                ret = PTR_ERR(ds18b20->dev);
                goto err_device_create;
        }

        ds18b20->gpiod = gpiod_get_optional(&pdev->dev, "ds18b20", GPIOD_OUT_HIGH);
        if (IS_ERR_OR_NULL(ds18b20->gpiod)) {
                if (ds18b20->gpiod == NULL)
                        ret = -ENODEV;
                else if (IS_ERR(ds18b20->gpiod))
                        ret = PTR_ERR(ds18b20->gpiod);
                goto err_get_gpio;
        }

        platform_set_drvdata(pdev, ds18b20);

        return 0;
err_get_gpio:
        device_destroy(ds18b20->class, ds18b20->dev_num);
err_device_create:
        class_destroy(ds18b20->class);
err_class:
        cdev_del(&ds18b20->cdev);
err_cdev:
        unregister_chrdev_region(ds18b20->dev_num, 1);
err_chrdev_region:
        kfree(ds18b20);
err_kzalloc:
        return ret;
}

static int ds18b20_remove(struct platform_device *pdev)
{
        struct ds18b20 *ds18b20 = platform_get_drvdata(pdev);

        gpiod_put(ds18b20->gpiod);
        device_destroy(ds18b20->class, ds18b20->dev_num);
        cdev_del(&ds18b20->cdev);
        class_destroy(ds18b20->class);
        unregister_chrdev_region(ds18b20->dev_num, 1);
        kfree(ds18b20);

        return 0;
}

const struct of_device_id match_table[] = {
        { .compatible = "even629,ds18b20" },
};

struct platform_driver ds18b20_pdrv = {
        .driver = {
                .name = "ds18b20",
                .owner = THIS_MODULE,
                .of_match_table = match_table,
                
        },
        .probe = ds18b20_probe,
        .remove = ds18b20_remove,
};

module_platform_driver(ds18b20_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for ds18b20");
