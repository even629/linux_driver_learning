#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#define TEST_GPIO_IN 101
static int irq;
static struct work_struct test_work;
static struct workqueue_struct *cmwq;

void test_work_func(struct work_struct *work)
{
        msleep(1000);
        pr_info("test_work_func is called\n");
}

irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        queue_work(cmwq, &test_work);
        return IRQ_HANDLED;
}

static int __init cmwq_test_init(void)
{
        int ret;
        irq = gpio_to_irq(TEST_GPIO_IN);
        if (irq < 0)
                return -ENODEV;

        ret = request_irq(irq, test_irq_handler, IRQF_TRIGGER_RISING, "test_irq", NULL);
        if (ret < 0)
                return -ENODEV;

        cmwq = alloc_workqueue("test_workqueue", WQ_UNBOUND | WQ_SYSFS, 0);
        if (cmwq == NULL) {
                free_irq(irq, NULL);
                return -EFAULT;
        }
        INIT_WORK(&test_work, test_work_func);

        return 0;
}

static void __exit cmwq_test_exit(void)
{
        free_irq(irq, NULL);
        flush_workqueue(cmwq);
        destroy_workqueue(cmwq);
}

module_init(cmwq_test_init);
module_exit(cmwq_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test for Concurrency Managed Work Queue");
