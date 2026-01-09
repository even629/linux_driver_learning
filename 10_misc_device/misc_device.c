#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>

static struct file_operations fops = {
        .owner = THIS_MODULE,
};
static struct miscdevice miscdev={
        .minor=MISC_DYNAMIC_MINOR,// 动态申请次设备号
        .name="miscdev",
        .fops = &fops,
};

static int __init misc_device_test_init(void)
{
        misc_register(&miscdev);
        return 0;
}

static void __exit misc_device_test_exit(void)
{
        misc_deregister(&miscdev);
}

module_init(misc_device_test_init);
module_exit(misc_device_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a misc device sample");
