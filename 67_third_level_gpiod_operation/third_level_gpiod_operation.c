#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>

int test_pdrv_probe(struct platform_device *pdev)
{
        int ret = 0;
        u32 count;
        struct device *dev = &pdev->dev;

        struct fwnode_handle *child = NULL;

        count = device_get_child_node_count(dev);
        if (count == 0) {
                dev_info(dev, "No child nodes\n");
                return 0;
        }

        dev_info(dev, "Found %u child nodes\n", count);

        while ((child = device_get_next_child_node(dev, child))) {
                struct gpio_desc *desc;
                desc = fwnode_get_named_gpiod(child, "my-gpios", 0, GPIOD_OUT_LOW,
                                              "test-child-gpio");
                if (IS_ERR(desc)) {
                        dev_warn(dev, "Skip child: failed to get GPIO (%ld)\n", PTR_ERR(desc));
                        continue;
                }
                gpiod_set_value(desc, 1);
                msleep(10);
                gpiod_set_value(desc, 0);

                gpiod_put(desc);
        }

        return ret;
}

int test_pdrv_remove(struct platform_device *pdev)
{
        return 0;
}

static const struct of_device_id match_table[] = { { .compatible = "even629,mygpio" } };

static struct platform_driver test_pdrv = {
        .driver = { .name = "third_level_test",
                    .owner = THIS_MODULE,
                    .of_match_table = match_table },
        .probe = test_pdrv_probe,
        .remove = test_pdrv_remove,
};

module_platform_driver(test_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for third level devicetree");
