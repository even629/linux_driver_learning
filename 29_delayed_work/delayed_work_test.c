#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define TEST_GPIO_PIN 101
static int irq;

void test_delayed_work_func(struct work_struct *work)
{
        pr_info("test_delayed_work_func is called\n");
        msleep(1000);
}

struct delayed_work test_delayed_work;

irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        // 提交到默认工作队列中
        schedule_delayed_work(&test_delayed_work, 3 * HZ);
        return IRQ_RETVAL(IRQ_HANDLED);
}

static int __init delayed_work_test_init(void)
{
        int ret;
        irq = gpio_to_irq(TEST_GPIO_PIN);

        if (irq < 0)
                return -ENODEV;

        ret = request_irq(irq, test_irq_handler, IRQF_TRIGGER_RISING, "test", NULL);

        if (ret < 0)
                return -ENODEV;

        INIT_DELAYED_WORK(&test_delayed_work, test_delayed_work_func);

        return 0;
}

static void __exit delayed_work_test_exit(void)
{
        free_irq(irq, NULL);
        // 确保工作完成后再退出
        flush_delayed_work(&test_delayed_work);
}

module_init(delayed_work_test_init);
module_exit(delayed_work_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test for delayed work");
