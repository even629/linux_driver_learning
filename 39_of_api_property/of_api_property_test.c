#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>

static int my_platform_driver_probe(struct platform_device *pdev)
{
        struct device_node *mydev_node;
        int i, num;
        u32 out_value_u32;
        u64 out_value_u64;
        u32 out_value_u32_array[2];
        const char *value_compatible;
        struct property *my_prop;

        pr_info("my_platform_driver_probe: Probing platform device\n");

        // 通过名字查找设备树节点
        mydev_node = of_find_node_by_name(NULL, "myLed");
        pr_info("[of_find_node_by_name]: device node is %s\n", mydev_node->name);

        // 查找compatible 属性
        my_prop = of_find_property(mydev_node, "compatible", NULL);
        pr_info("[of_find_property]: property name is %s\n", my_prop->name);

        // 获取reg 属性的元素的数量
        num = of_property_count_elems_of_size(mydev_node, "reg", sizeof(u32));
        pr_info("[of_property_count_elems_of_size]: reg elem size is %d\n", num);

        // 读取 reg 属性的值 u32
        for (i = 0; i < num; i++) {
                of_property_read_u32_index(mydev_node, "reg", i, &out_value_u32);
                pr_info("[of_property_read_u32_index]: reg u32 value: 0x%X\n", out_value_u32);
        }

        // 读取 reg 属性的值 u64
        of_property_read_u64_index(mydev_node, "reg", 0, &out_value_u64);
        pr_info("[of_property_read_u64_index]: reg u64 value: 0x%llX\n", out_value_u64);

        // 读取 reg 属性为一个数组
        of_property_read_variable_u32_array(mydev_node, "reg", out_value_u32_array, 1, 2);
        pr_info("[of_property_read_variable_u32_array]: array[0] is 0x%X\n",
                out_value_u32_array[0]);
        pr_info("[of_property_read_variable_u32_array]: array[1] is 0x%X\n",
                out_value_u32_array[1]);

        // 读取 compatible 属性的字符串值
        of_property_read_string(mydev_node, "compatible", &value_compatible);
        pr_info("[of_property_read_string]: compatible string is %s\n", value_compatible);

        return 0;
}

static int my_platform_driver_remove(struct platform_device *pdev)
{
        return 0;
}

static const struct of_device_id of_match_table[] = { { .compatible = "my devicetree" }, {} };

static struct platform_driver my_platform_driver={
        .driver={
                .owner = THIS_MODULE,
                .name = "my_platform_driver",
                .of_match_table = of_match_table,
        },
        .probe = my_platform_driver_probe,
        .remove = my_platform_driver_remove,
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("this is a test sample for devicetree api: of_property");
