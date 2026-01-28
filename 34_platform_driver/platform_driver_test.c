#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

static int platform_driver_test_probe(struct platform_device *pdev)
{
        struct resource *res_mem, *res_irq;
        // Method1: 直接访问
        if (pdev->num_resources >= 2) {
                struct resource *res_mem = &pdev->resource[0];
                struct resource *res_irq = &pdev->resource[1];
                pr_info("using pdev->resource[i] to get resource\n");
                pr_info("Memory Resource: start=0x%llx, end=0x%llx\n", res_mem->start,
                        res_mem->end);
                pr_info("IRQ Resource: number=%lld\n", res_irq->start);
        }
        // Method 2:使用platform_get_resource()
        res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (res_mem == NULL) {
                dev_err(&pdev->dev, "Fail to get MEMORY resource\n");
                return -ENODEV;
        }
        res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        if (res_irq == NULL) {
                dev_err(&pdev->dev, "Fail to get IRQ resource\n");
                return -ENODEV;
        }
        pr_info("using platform_get_resource() to get resource\n");
        pr_info("Memory Resource: start=0x%llx, end=0x%llx\n", res_mem->start, res_mem->end);
        pr_info("IRQ Resource: number=%lld\n", res_irq->start);

        return 0;
}
static int platform_driver_test_remove(struct platform_device *pdev)
{
        pr_info("platform_driver_test_remove is called\n");
        return 0;
}

static struct platform_driver my_platform_driver={
        .driver = {
                .name = "my_platform_device",
                .owner = THIS_MODULE,
        },
        .probe = platform_driver_test_probe,
        .remove = platform_driver_test_remove,
};

static int __init platform_driver_test_init(void)
{
        int ret;

        ret = platform_driver_register(&my_platform_driver);
        if (ret < 0) {
                pr_err("platform_driver_register failed\n");
                return ret;
        }

        pr_info("platform_driver_register success\n");
        return 0;
}

static void __exit platform_driver_test_exit(void)
{
        platform_driver_unregister(&my_platform_driver);
}

module_init(platform_driver_test_init);
module_exit(platform_driver_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for platform_driver");
