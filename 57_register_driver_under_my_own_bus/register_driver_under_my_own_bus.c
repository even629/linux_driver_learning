#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>

extern struct bus_type mybus;

int mydriver_probe(struct device *dev)
{
        pr_info("%s\n", __func__);
        return 0;
}

int mydriver_remove(struct device *dev)
{
        pr_info("%s\n", __func__);
        return 0;
}

struct device_driver mydriver = {
        .name = "mydevice",
        .bus = &mybus,
        .probe = mydriver_probe,
        .remove = mydriver_remove,
};

static int __init mybus_driver_test_init(void)
{
        int ret;

        ret = driver_register(&mydriver);
        return ret;
}

static void __exit mybus_driver_test_exit(void)
{
        driver_unregister(&mydriver);
}

module_init(mybus_driver_test_init);
module_exit(mybus_driver_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is test description for bus");
