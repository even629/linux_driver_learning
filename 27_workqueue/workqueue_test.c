#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define TEST_GPIO_PIN 101

static int irq;
struct work_struct test_work;

void test_work_func(struct work_struct *work)
{
        msleep(1000);
        pr_info("test_work_func is called\n");
}

irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        // 中断下半部
        // 提交工作项到工作队列
        schedule_work(&test_work);
        return IRQ_RETVAL(IRQ_HANDLED);
}

static int __init workqueue_test_init(void)
{
        int ret;
        irq = gpio_to_irq(TEST_GPIO_PIN);
        if (irq < 0)
                return -ENODEV;
        pr_info("irq is %d\n", irq);
        ret = request_irq(irq, test_irq_handler, IRQF_TRIGGER_RISING, "test", NULL);
        if (ret < 0) {
                free_irq(irq, NULL);
                return -ENODEV;
        }

        INIT_WORK(&test_work, test_work_func);

        return 0;
}

static void __exit workqueue_test_exit(void)
{
        free_irq(irq, NULL);
        // 模块退出前必须保证所有已经提交的工作项已经完成
        flush_work(&test_work);
}

module_init(workqueue_test_init);
module_exit(workqueue_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test for workqueue");
