#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>

int mybus_match(struct device *dev, struct device_driver *drv)
{
        return (strcmp(dev_name(dev), drv->name) == 0);
}

int mybus_probe(struct device *dev)
{
        // struct device_driver *drv = dev->driver;
        // if (drv->probe)
        //         drv->probe(dev);

        pr_info("%s\n", __func__);
        return 0;
}

ssize_t mybus_show(struct bus_type *bus, char *buf)
{
        // 在 sysfs 中显示总线的值
        return sprintf(buf, "%s\n", "mybus_show");
}

ssize_t mybus_store(struct bus_type *bus, const char *buf, size_t count)
{
        pr_info("nothing done\n");
        return count;
}

struct bus_attribute mybus_attr = {
        .attr = {
                .name = "value",
                .mode = 0644,
        },
        .show = mybus_show,
        .store = mybus_store,        
};

struct bus_type mybus = {
        .name = "mybus",
        .match = mybus_match,
        .probe = mybus_probe,
};
EXPORT_SYMBOL_GPL(mybus); // 导出总线符号

static int __init my_own_bus_test_init(void)
{
        int ret;
        ret = bus_register(&mybus);
        ret = bus_create_file(&mybus, &mybus_attr);
        return ret;
}

static void __exit my_own_bus_test_exit(void)
{
        bus_remove_file(&mybus, &mybus_attr);
        bus_unregister(&mybus);
}

module_init(my_own_bus_test_init);
module_exit(my_own_bus_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for bus");
