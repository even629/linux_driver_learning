#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>

/* 复用寄存器 */
#define PMU_GRF_BASE 0xFDC20000
#define GPIO0B_IOMUX_OFFSET 0xC
#define GPIO0B_IOMUX PMU_GRF_BASE + GPIO0B_IOMUX_OFFSET

/* 数据寄存器与方向寄存器 */
#define GPIO0_BASE 0xFDD60000
#define GPIO_SWPORT_DDR_L_OFFSET 0x8
#define GPIO_SWPORT_DR_L_OFFSET 0x0
#define GPIO_EXT_PORT_OFFSET 0x70
// 方向寄存器
#define GPIO0_SWPORT_DDR_L GPIO0_BASE + GPIO_SWPORT_DDR_L_OFFSET
// 数据寄存器
#define GPIO0_SWPORT_DR_L GPIO0_BASE + GPIO_SWPORT_DR_L_OFFSET
// 外部输入寄存器,readonly
#define GPIO0_EXT_PORT GPIO0_BASE + GPIO_EXT_PORT_OFFSET

struct led_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        void __iomem *gpio0b_iomux;
        void __iomem *gpio0_swport_ddr_l;
        void __iomem *gpio0_swport_dr_l;
        void __iomem *gpio0_ext_port;
        char read_kbuf[4];
        char write_kbuf[4];
};

static struct led_drv_data *led_drv_data;

int light_up_led_open(struct inode *inode, struct file *file)
{
        file->private_data = led_drv_data;
        return 0;
}

ssize_t light_up_led_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct led_drv_data *drv_data = file->private_data;
        size_t len = min(size, (size_t)((loff_t)4 - *offset));
        u32 val;

        if (*offset == 0) {
                val = readl(drv_data->gpio0_ext_port);
                drv_data->read_kbuf[0] = (val >> 24) & 0xFF;
                drv_data->read_kbuf[1] = (val >> 16) & 0xFF;
                drv_data->read_kbuf[2] = (val >> 8) & 0xFF;
                drv_data->read_kbuf[3] = val & 0xFF;
        }

        if (copy_to_user(buf, drv_data->read_kbuf + *offset, len) != 0) {
                return -EFAULT;
        }

        *offset += len;

        return len;
}
ssize_t light_up_led_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        struct led_drv_data *drv_data = file->private_data;
        size_t len = min(size, (size_t)((loff_t)4 - *offset));
        u32 val;

        if (copy_from_user(led_drv_data->write_kbuf + *offset, buf, len) != 0) {
                return -EFAULT;
        }

        *offset += len;

        if (*offset >= 4) {
                val = readl(drv_data->gpio0_swport_ddr_l);
                val |= 0x40004000;
                writel(val, drv_data->gpio0_swport_ddr_l);
                
                val = readl(drv_data->gpio0_swport_dr_l);
                val |= 0x40004000;
                writel(val, drv_data->gpio0_swport_dr_l);
        }

        return len;
}

int light_up_led_release(struct inode *inode, struct file *file)
{
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = light_up_led_open,
        .read = light_up_led_read,
        .write = light_up_led_write,
        .release = light_up_led_release,
};

static int __init light_up_led_init(void)
{
        int err;
        u32 val;

        led_drv_data = kzalloc(sizeof(struct led_drv_data), GFP_KERNEL);
        if (led_drv_data == NULL) {
                err = -ENOMEM;
                goto kzalloc_fail;
        }
        err = alloc_chrdev_region(&led_drv_data->dev_num, 0, 1, "led chrdev region");
        if (err < 0)
                goto alloc_chrdev_region_fail;

        cdev_init(&led_drv_data->cdev, &fops);
        led_drv_data->cdev.owner = THIS_MODULE;
        err = cdev_add(&led_drv_data->cdev, led_drv_data->dev_num, 1);
        if (err < 0)
                goto cdev_add_fail;

        led_drv_data->class = class_create(THIS_MODULE, "test_led");
        if (IS_ERR(led_drv_data->class)) {
                err = PTR_ERR(led_drv_data->class);
                goto create_class_fail;
        }
        led_drv_data->dev = device_create(led_drv_data->class, NULL, led_drv_data->dev_num, NULL,
                                          "test_led%d", 0);
        if (IS_ERR(led_drv_data->dev)) {
                err = PTR_ERR(led_drv_data->dev);
                goto device_create_fail;
        }

        led_drv_data->gpio0b_iomux = ioremap(GPIO0B_IOMUX, 4);
        led_drv_data->gpio0_swport_ddr_l = ioremap(GPIO0_SWPORT_DDR_L, 4);
        led_drv_data->gpio0_swport_dr_l = ioremap(GPIO0_SWPORT_DR_L, 4);
        led_drv_data->gpio0_ext_port = ioremap(GPIO0_EXT_PORT, 4);

        // 设置引脚复用为GPIO
        val = readl(led_drv_data->gpio0b_iomux);
        val |= 0x70000000; // write access
        val &= 0xFFFF8FFF; // gpio0_b7
        writel(val, led_drv_data->gpio0b_iomux);

        return 0;
device_create_fail:
        class_destroy(led_drv_data->class);
create_class_fail:
        cdev_del(&led_drv_data->cdev);
cdev_add_fail:
        unregister_chrdev_region(led_drv_data->dev_num, 1);
alloc_chrdev_region_fail:
        kfree(led_drv_data);
kzalloc_fail:
        return err;
}

static void __exit light_up_led_exit(void)
{
        iounmap(led_drv_data->gpio0_ext_port);
        iounmap(led_drv_data->gpio0b_iomux);
        iounmap(led_drv_data->gpio0_swport_ddr_l);
        iounmap(led_drv_data->gpio0_swport_dr_l);

        device_destroy(led_drv_data->class, led_drv_data->dev_num);
        class_destroy(led_drv_data->class);
        cdev_del(&led_drv_data->cdev);
        unregister_chrdev_region(led_drv_data->dev_num, 1);
        kfree(led_drv_data);
}
module_init(light_up_led_init);
module_exit(light_up_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("light up a led on topeet RK3568 board");
