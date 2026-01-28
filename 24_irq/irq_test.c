#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define TEST_GPIO_PIN 101

struct irq_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
};

static struct irq_drv_data *drv_dat;

struct file_operations fops = {
        .owner = THIS_MODULE,
};

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
        pr_info("Interrupt occurred on GPIO %d\n", TEST_GPIO_PIN);
        return IRQ_HANDLED;
}

static int __init irq_test_init(void)
{
        int ret, irq_num;
        drv_dat = (struct irq_drv_data *)kzalloc(sizeof(struct irq_drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }
        ret = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "test_chrdev_region");
        if (ret < 0)
                goto chrdev_region_fail;

        cdev_init(&drv_dat->cdev, &fops);
        drv_dat->cdev.owner = THIS_MODULE;
        ret = cdev_add(&drv_dat->cdev, drv_dat->dev_num, 1);
        if (ret < 0)
                goto cdev_add_fail;

        drv_dat->class = class_create(THIS_MODULE, "test");
        if (IS_ERR(drv_dat->class)) {
                ret = PTR_ERR(drv_dat->class);
                goto class_create_fail;
        }
        drv_dat->dev = device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "test_irq%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                ret = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        // 申请中断
        irq_num = gpio_to_irq(TEST_GPIO_PIN);
        pr_info("GPIO %d mapped to IRQ %d\n", TEST_GPIO_PIN, irq_num);
        if (irq_num < 0){
                ret = -ENODEV;
                goto gpio_to_irq_fail;
        }
                

        if (request_irq(irq_num, gpio_irq_handler, IRQF_TRIGGER_RISING, "irq_test", NULL) != 0) {
                pr_err("fail to request IRQ %d\n", irq_num);
                ret = -ENODEV;
                goto request_irq_fail;
        }

        return 0;
request_irq_fail:
        gpio_free(TEST_GPIO_PIN);
gpio_to_irq_fail:
        device_destroy(drv_dat->class, drv_dat->dev_num);
device_create_fail:
        class_destroy(drv_dat->class);
class_create_fail:
        cdev_del(&drv_dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(drv_dat->dev_num, 1);
chrdev_region_fail:
        kfree(drv_dat);
kzalloc_fail:
        return ret;
}

static void __exit irq_test_exit(void)
{
        gpio_free(TEST_GPIO_PIN);
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(irq_test_init);
module_exit(irq_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("test sample for irq");
