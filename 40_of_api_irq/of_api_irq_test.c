#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

static int my_platform_driver_probe(struct platform_device *pdev)
{        
        int irq;
        struct irq_data *my_irq_data;
        struct device_node *mydev_node;
        u32 trigger_type;
        pr_info("my_platform_probe: Probing platform device\n");

        // 获取设备节点
        mydev_node = pdev->dev.of_node;

        // 解析和映射中断
        irq = irq_of_parse_and_map(mydev_node, 0);
        pr_info("[irq_of_parse_and_map]: irq is %d\n", irq);

        // 获取中断数据结构
        my_irq_data = irq_get_irq_data(irq);
        // 获取中断出发类型
        trigger_type = irqd_get_trigger_type(my_irq_data);
        pr_info("[irqd_get_trigger_type]: trigger_type is %d\n", trigger_type);

        // 将 gpio转为中断号
        irq = gpio_to_irq(101);
        pr_info("[gpio_to_irq]: irq is %d\n", irq);

        // 从设备节点获取中断号
        irq = of_irq_get(mydev_node, 0);
        pr_info("[of_irq_get]: irq is %d\n", irq);

        // 获取平台设备的中断号
        irq = platform_get_irq(pdev, 0);
        pr_info("[platform_get_irq]: irq is %d\n", irq);
               
        return 0;
}

static int my_platform_driver_remove(struct platform_device *pdev)
{
        return 0;
}

static const struct of_device_id of_match_table[] = { { .compatible = "my irq" }, {} };

static struct platform_driver my_platform_drv = {
        .driver = {
                .owner = THIS_MODULE,
                .name = "my platform driver",
                .of_match_table = of_match_table,
        },
        .probe = my_platform_driver_probe,
        .remove = my_platform_driver_remove,
};

module_platform_driver(my_platform_drv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for platform irq");
