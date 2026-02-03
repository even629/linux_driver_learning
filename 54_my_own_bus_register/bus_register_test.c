#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>

int mybus_match(struct device *dev, struct device_driver *drv)
{
        return (strcmp(dev_name(dev), drv->name) == 0);
}

int mybus_probe(struct device *dev)
{
        struct device_driver *drv = dev->driver;
        if (drv->probe)
                drv->probe(dev);

        return 0;
}

struct bus_type mybus_type = {
        .name = "mybus",
        .match = mybus_match,
        .probe = mybus_probe,
};

static int __init my_own_bus_test_init(void)
{
        int ret;
        ret = bus_register(&mybus_type);
        return ret;
}

static void __exit my_own_bus_test_exit(void)
{
        bus_unregister(&mybus_type);
}

module_init(my_own_bus_test_init);
module_exit(my_own_bus_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for bus");
