#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>

extern struct bus_type mybus;

void mydev_release(struct device *dev)
{
        pr_info("%s\n", __func__);
}

struct device mydevice = {
        .init_name = "mydevice",        
        .bus = &mybus,
        .release = mydev_release,
        .devt = ((255 << 20) | 0),
};

static int __init device_test_init(void)
{
        int ret;
        ret = device_register(&mydevice);
        return ret;
}

static void __exit device_test_exit(void)
{
        device_unregister(&mydevice);
}

module_init(device_test_init);
module_exit(device_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for my own bus");
