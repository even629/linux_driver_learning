#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define TEST_GPIO_PIN 101

static int irq;
static struct tasklet_struct mytasklet;

irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        tasklet_schedule(&mytasklet);
        return IRQ_RETVAL(IRQ_HANDLED);
}
void mytasklet_func(unsigned long data)
{
        pr_info("data is %lu\n", data);
}

static int __init tasklet_test_init(void)
{
        int ret;
        irq = gpio_to_irq(TEST_GPIO_PIN);
        pr_info("irq is %d\n", irq);
        if (irq < 0)
                return -ENODEV;
        ret = request_irq(irq, test_irq_handler, IRQF_TRIGGER_RISING, "test", NULL);
        if (ret < 0)
                return -ENODEV;

        tasklet_init(&mytasklet, mytasklet_func, 1);
        tasklet_enable(&mytasklet);// optional: init之后默认是enable的

        return 0;
}

static void __exit tasklet_test_exit(void)
{
        tasklet_disable(&mytasklet);
        tasklet_kill(&mytasklet);
        free_irq(irq, NULL);
        printk("bye\n");
}

module_init(tasklet_test_init);
module_exit(tasklet_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for tasklet");
