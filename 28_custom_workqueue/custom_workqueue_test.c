#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define TEST_GPIO_PIN 101

static int irq;

static struct workqueue_struct *test_wq;
static struct work_struct test_work;
// 中断下半部，工作处理函数
irqreturn_t test_irq_handler(int irq, void *dev_id)
{
        pr_info("test_irq_handler is called\n");
        queue_work(test_wq, &test_work);
        // queue_work_on(0, test_wq, &test_work);
        return IRQ_RETVAL(IRQ_HANDLED);
}
// 中断上半部，中断处理函数
void test_work_func(struct work_struct *work)
{
        msleep(1000);
        pr_info("test_work_func is called\n");
}

static int __init custom_workqueue_test_init(void)
{
        int ret;

        irq = gpio_to_irq(TEST_GPIO_PIN);
        if (irq < 0) {
                ret = -ENODEV;
                goto get_irq_fail;
        }

        ret = request_irq(irq, test_irq_handler, IRQF_TRIGGER_RISING, "test", NULL);
        if (ret < 0)
                goto request_irq_fail;

        // 创建工作队列
        test_wq = create_workqueue("test");
        if (IS_ERR_OR_NULL(test_wq))
                goto create_workqueue_fail;
        // 初始化工作项
        INIT_WORK(&test_work, test_work_func);

        return 0;

create_workqueue_fail:
        free_irq(irq, NULL);
request_irq_fail:
get_irq_fail:
        return ret;
}

static void __exit custom_workqueue_test_exit(void)
{
        free_irq(irq, NULL);// 禁止新中断
        // 取消工作项       
        // flush_work(&test_work);
        // 刷新工作队列，等待所有已经提交但尚未执行的工作完成
        flush_workqueue(test_wq);
        // 销毁工作队列释放资源
        destroy_workqueue(test_wq);
        
}

module_init(custom_workqueue_test_init);
module_exit(custom_workqueue_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test for custom workqueue");
