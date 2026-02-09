#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>

struct input_dev *myinput_dev;

static int __init myinput_dev_test_init(void)
{
        int ret = 0;

        myinput_dev = input_allocate_device();
        if (!myinput_dev) {
                pr_info("input_allocate_device error\n");
                return -EFAULT;
        }
        // 设置输入设备的名称
        myinput_dev->name = "myinput_dev";
        // 设置输入设备的事件类型

        __set_bit(EV_KEY, myinput_dev->evbit);
        __set_bit(KEY_1, myinput_dev->keybit);

        // 设置输入设备支持的事件类型
        ret = input_register_device(myinput_dev);
        if(ret < 0){
                pr_info("input_register_dev error\n");
                goto error;
        }

        return 0;
 error:
        input_free_device(myinput_dev);
        return ret;
}

static void __exit myinput_dev_test_exit(void)
{
        input_unregister_device(myinput_dev);
}

module_init(myinput_dev_test_init);
module_exit(myinput_dev_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for input dev");
