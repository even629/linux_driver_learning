#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h> // struct resource

#define MEM_START_ADDR 0xFDD60000
#define MEM_END_ADDR 0xFDD60004
#define IRQ_NUMBER 101

static struct resource my_resources[] = { {
                                                  .start = MEM_START_ADDR,
                                                  .end = MEM_END_ADDR,
                                                  .name = "test_resource1",
                                                  .flags = IORESOURCE_MEM, // 标记为内存资源
                                          },
                                          {
                                                  .start = IRQ_NUMBER,
                                                  .end = IRQ_NUMBER,
                                                  .flags = IORESOURCE_IRQ, // 标记为中断资源
                                          } };

void my_dev_release(struct device *dev) // pdev->dev资源释放的回调函数
{
        pr_info("my_dev_release is called\n");
}

static struct platform_device my_platform_device = {
        .name = "my_platform_device",
        .id = -1, // 设备id
        .num_resources = ARRAY_SIZE(my_resources),
        .resource = my_resources,
        .dev.release = my_dev_release,
};

static int __init platform_device_test_init(void)
{
        int ret;
        ret = platform_device_register(&my_platform_device);
        if (ret < 0){
                pr_err("platform_device_register fail\n");
                return ret;
        }
        pr_info("platform_device register success\n");
                

        return 0;
}

static void __exit platform_device_test_exit(void)
{
        platform_device_unregister(&my_platform_device);
}

module_init(platform_device_test_init);
module_exit(platform_device_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for platform_device");
