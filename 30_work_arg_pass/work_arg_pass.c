#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define TEST_GPIO_PIN 101

struct drv_data {
        int irq;
        struct work_struct test_work;
        int arg;
};

static struct drv_data *drv_dat;

void test_work_func(struct work_struct *work)
{
        struct drv_data *drv_dat = container_of(work, struct drv_data, test_work);
        pr_info("test_work_func is called, arg is %d\n", drv_dat->arg);
        msleep(1000);
}

irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        schedule_work(&drv_dat->test_work);
        return IRQ_RETVAL(IRQ_HANDLED);
}

static int __init test_arg_pass_init(void)
{
        int ret;
        drv_dat = kzalloc(sizeof(struct drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }
        drv_dat->irq = gpio_to_irq(TEST_GPIO_PIN);
        if (drv_dat->irq < 0) {
                ret = -ENODEV;
                goto fail;
        }

        ret = request_irq(drv_dat->irq, test_irq_handler, IRQF_TRIGGER_RISING, "test", NULL);
        if (ret < 0) {
                ret = -ENODEV;
                goto fail;
        }
        drv_dat->arg = 0x6;
        INIT_WORK(&drv_dat->test_work, test_work_func);

        return 0;
fail:
        kfree(drv_dat);
kzalloc_fail:
        return ret;
}

static void __exit test_arg_pass_exit(void)
{
        free_irq(drv_dat->irq, NULL);
        flush_work(&drv_dat->test_work);
        kfree(drv_dat);
}

module_init(test_arg_pass_init);
module_exit(test_arg_pass_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test for passing args to work func");
