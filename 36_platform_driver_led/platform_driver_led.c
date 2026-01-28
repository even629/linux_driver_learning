#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioctl.h>
struct led_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        void __iomem *gpio0b_iomux;
        void __iomem *gpio0_swport_ddr_l;
        void __iomem *gpio0_swport_dr_l;
        void __iomem *gpio0_ext_port;
};

int led_open(struct inode *inode, struct file *file)
{
        struct cdev *led_cdev = inode->i_cdev;
        file->private_data = container_of(led_cdev, struct led_drv_data, cdev);
        pr_info("led_open is called\n");
        return 0;
}
ssize_t led_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct led_drv_data *drv_data = file->private_data;
        u32 val;

        if (size != sizeof(u32)) {
                pr_info("read need 4 bytes\n");
                return -ENOMEM;
        }

        val = readl(drv_data->gpio0_ext_port);
        val &= 1 << 15; // gpio0_b7
        val = val >> 15;

        if (copy_to_user(buf, &val, sizeof(u32)) != 0) {
                return -EFAULT;
        }
        pr_info("led_read is called\n");
        return sizeof(u32);
}
ssize_t led_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        struct led_drv_data *drv_data = file->private_data;
        u32 val;

        if (size != sizeof(u32)) {
                pr_err("write need 4 bytes\n");
                return -ENOMEM;
        }

        if (copy_from_user(&val, buf, sizeof(u32)) != 0) {
                return -EFAULT;
        }
        if (val > 0) {
                // 配置为GPIO输出
                val = readl(drv_data->gpio0_swport_ddr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_ddr_l);
                // 打开
                val = readl(drv_data->gpio0_swport_dr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_dr_l);
        } else if (val == 0) {
                // 配置为GPIO输出
                val = readl(drv_data->gpio0_swport_ddr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_ddr_l);
                // 关闭
                val = readl(drv_data->gpio0_swport_dr_l);
                val |= 0x80000000;
                val &= 0xffff7fff;
                writel(val, drv_data->gpio0_swport_dr_l);
        }
        pr_info("led_write is called\n");
        return sizeof(u32);
}

#define LED_OPEN _IO('A', 0)
#define LED_CLOSE _IO('A', 1)
#define LED_STATUS _IOR('A', 2, u32)

long led_unlocked_ioctl(struct file *file, unsigned int op, unsigned long arg)
{
        struct led_drv_data *drv_data = file->private_data;
        int val;
        switch (op) {
        case LED_OPEN:
                // 配置为GPIO输出
                val = readl(drv_data->gpio0_swport_ddr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_ddr_l);
                // 打开
                val = readl(drv_data->gpio0_swport_dr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_dr_l);
                break;
        case LED_CLOSE:
                // 配置为GPIO输出
                val = readl(drv_data->gpio0_swport_ddr_l);
                val |= 0x80008000;
                writel(val, drv_data->gpio0_swport_ddr_l);
                // 关闭
                val = readl(drv_data->gpio0_swport_dr_l);
                val |= 0x80000000;
                val &= 0xffff7fff;
                writel(val, drv_data->gpio0_swport_dr_l);
                break;
        case LED_STATUS:
                val = readl(drv_data->gpio0_ext_port);
                val &= 1 << 15; // gpio0_b7
                val = val >> 15;
                if (copy_to_user((void __user *)arg, &val, sizeof(val)))
                        return -EFAULT;
                break;
        default:
                return -EFAULT;
        }
        pr_info("led_ioctl is called\n");
        return 0;
}

int led_release(struct inode *inode, struct file *file)
{
        pr_info("led_release is called\n");
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = led_open,
        .read = led_read,
        .write = led_write,
        .unlocked_ioctl = led_unlocked_ioctl,
        .release = led_release,
};

static int my_platform_driver_probe(struct platform_device *pdev)
{
        int ret;
        u32 val;
        struct led_drv_data *led_drv_dat;
        struct resource *res_iomux, *res_ddr_l, *res_dr_l, *res_ext_port;

        led_drv_dat = devm_kzalloc(&pdev->dev, sizeof(struct led_drv_data), GFP_KERNEL);

        if (led_drv_dat == NULL) {
                ret = -ENOMEM;
                goto devm_kzalloc_fail;
        }

        ret = alloc_chrdev_region(&led_drv_dat->dev_num, 0, 1, "lightup_led_chrdev_region");
        if (ret < 0) {
                ret = -ENOMEM;
                goto alloc_chrdev_region_fail;
        }
        platform_set_drvdata(pdev, led_drv_dat);

        cdev_init(&led_drv_dat->cdev, &fops);
        led_drv_dat->cdev.owner = THIS_MODULE;
        ret = cdev_add(&led_drv_dat->cdev, led_drv_dat->dev_num, 1);
        if (ret < 0)
                goto cdev_add_fail;

        led_drv_dat->class = class_create(THIS_MODULE, "test_led");
        if (IS_ERR(led_drv_dat->class)) {
                ret = PTR_ERR(led_drv_dat->class);
                goto class_create_fail;
        }

        led_drv_dat->dev =
                device_create(led_drv_dat->class, NULL, led_drv_dat->dev_num, NULL, "led0");
        if (IS_ERR(led_drv_dat->dev)) {
                ret = PTR_ERR(led_drv_dat->dev);
                goto device_create_fail;
        }

        res_iomux = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        res_ddr_l = platform_get_resource(pdev, IORESOURCE_MEM, 1);
        res_dr_l = platform_get_resource(pdev, IORESOURCE_MEM, 2);
        res_ext_port = platform_get_resource(pdev, IORESOURCE_MEM, 3);

        if (!res_iomux || !res_ddr_l || !res_dr_l || !res_ext_port) {
                ret = -ENODEV;
                goto address_fail;
        }

        led_drv_dat->gpio0b_iomux =
                devm_ioremap(&pdev->dev, res_iomux->start, resource_size(res_iomux));
        led_drv_dat->gpio0_swport_ddr_l =
                devm_ioremap(&pdev->dev, res_ddr_l->start, resource_size(res_ddr_l));
        led_drv_dat->gpio0_swport_dr_l =
                devm_ioremap(&pdev->dev, res_dr_l->start, resource_size(res_dr_l));
        led_drv_dat->gpio0_ext_port =
                devm_ioremap(&pdev->dev, res_ext_port->start, resource_size(res_ext_port));

        if (!led_drv_dat->gpio0b_iomux || !led_drv_dat->gpio0_swport_ddr_l ||
            !led_drv_dat->gpio0_swport_dr_l || !led_drv_dat->gpio0_ext_port) {
                ret = -ENOMEM;
                goto address_fail;
        }

        // led_drv_dat->gpio0b_iomux =
        //         devm_ioremap_resource(&pdev->dev, platform_get_resource(pdev, IORESOURCE_MEM, 0));
        // if (IS_ERR(led_drv_dat->gpio0b_iomux)) {
        //         ret = PTR_ERR(led_drv_dat->gpio0b_iomux);
        //         goto ioremap_fail;
        // }

        // led_drv_dat->gpio0_swport_ddr_l =
        //         devm_ioremap_resource(&pdev->dev, platform_get_resource(pdev, IORESOURCE_MEM, 1));
        // if (IS_ERR(led_drv_dat->gpio0_swport_ddr_l)) {
        //         ret = PTR_ERR(led_drv_dat->gpio0_swport_ddr_l);
        //         goto ioremap_fail;
        // }

        // led_drv_dat->gpio0_swport_dr_l =
        //         devm_ioremap_resource(&pdev->dev, platform_get_resource(pdev, IORESOURCE_MEM, 2));
        // if (IS_ERR(led_drv_dat->gpio0_swport_dr_l)) {
        //         ret = PTR_ERR(led_drv_dat->gpio0_swport_dr_l);
        //         goto ioremap_fail;
        // }

        // led_drv_dat->gpio0_ext_port =
        //         devm_ioremap_resource(&pdev->dev, platform_get_resource(pdev, IORESOURCE_MEM, 3));
        // if (!led_drv_dat->gpio0_ext_port) {
        //         ret = -ENODEV;
        //         goto ioremap_fail;
        // }

        // 设置引脚复用为GPIO
        val = readl(led_drv_dat->gpio0b_iomux);
        val |= 0x70000000; // write access
        val &= 0xFFFF8FFF; // gpio0_b7
        writel(val, led_drv_dat->gpio0b_iomux);
        pr_info("gpio0b_7 is set as GPIO\n");

        return 0;
address_fail:
        device_destroy(led_drv_dat->class, led_drv_dat->dev_num);
device_create_fail:
        class_destroy(led_drv_dat->class);
class_create_fail:
        cdev_del(&led_drv_dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(led_drv_dat->dev_num, 1);
alloc_chrdev_region_fail:
devm_kzalloc_fail:
        return ret;
}
static int my_platform_driver_remove(struct platform_device *pdev)
{
        struct led_drv_data *led_drv_dat = platform_get_drvdata(pdev);
        device_destroy(led_drv_dat->class, led_drv_dat->dev_num);
        class_destroy(led_drv_dat->class);
        cdev_del(&led_drv_dat->cdev);
        unregister_chrdev_region(led_drv_dat->dev_num, 1);
        return 0;
}

static struct platform_driver my_platform_driver = {
        .driver ={
                .name = "light_up_led",
                .owner = THIS_MODULE,
        },
        .probe = my_platform_driver_probe,
        .remove = my_platform_driver_remove,               
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is light up led example for platform bus");
