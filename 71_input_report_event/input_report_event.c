#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/timer.h>

struct input_dev *myinput_dev;
static int value = 0;

static void test_timer_func(struct timer_list *t)
{
        value = !value;

        input_event(myinput_dev, EV_KEY, KEY_1, value); // 上报按键事件
        input_sync(myinput_dev); // 发送同步事件

        mod_timer(t, jiffies + msecs_to_jiffies(1000)); // 更新定时器
}

DEFINE_TIMER(test_timer, test_timer_func);

static int input_report_event_test_init(void)
{
        int ret;

        myinput_dev = input_allocate_device();
        if (myinput_dev == NULL)
                return -ENOMEM;
        myinput_dev->name = "myinput_dev";

        set_bit(EV_KEY, myinput_dev->evbit);
        set_bit(EV_SYN, myinput_dev->evbit);
        set_bit(KEY_1, myinput_dev->keybit);

        ret = input_register_device(myinput_dev);
        if (ret < 0)
                goto err;

        mod_timer(&test_timer, jiffies + msecs_to_jiffies(1000));

        return 0;

err:
        input_free_device(myinput_dev);
        return ret;
}

static void input_report_event_test_exit(void)
{
        del_timer_sync(&test_timer);
        input_unregister_device(myinput_dev);
}

module_init(input_report_event_test_init);
module_exit(input_report_event_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for input subsys");
