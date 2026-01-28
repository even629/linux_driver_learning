#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define TEST_GPIO_PIN 101

static int irq;

irqreturn_t test_irq_top_half_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        // 将中断工作推迟到中断下半部，由内核线程处理
        return IRQ_WAKE_THREAD;
}
irqreturn_t test_irq_bottom_half_handler(int irq, void *dev_id)
{
        msleep(1000);
        pr_info("threaded_irq_handler is called\n");
        return IRQ_HANDLED;
}

static int __init threaded_irq_test_init(void)
{
        int ret;
        irq = gpio_to_irq(TEST_GPIO_PIN);
        if (irq < 0)
                return -ENODEV;
        ret = request_threaded_irq(irq, test_irq_top_half_handler, test_irq_bottom_half_handler,
                                   IRQF_TRIGGER_RISING, "test", NULL);
        if (ret < 0)
                return ret;
        return 0;
}

static void __exit threaded_irq_test_exit(void)
{
        free_irq(irq, NULL);
}

module_init(threaded_irq_test_init);
module_exit(threaded_irq_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for threaded irq");
