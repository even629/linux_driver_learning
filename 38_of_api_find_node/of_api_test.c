#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
static const struct of_device_id mynode_of_match[] = { { .compatible = "my devicetree" }, {} };

static int my_platform_driver_probe(struct platform_device *pdev)
{
        struct device_node *mydev_node;
        pr_info("my_platform_driver_probe: Probing platform device\n");

        // 通过node name查找设备树接单
        mydev_node = of_find_node_by_name(NULL, "myLed");
        pr_info("[of_find_node_by_name]: device node is %s\n", mydev_node->name);

        // 通过节点路径查找设备树节点
        mydev_node = of_find_node_by_path("/test_device/myLed");
        pr_info("[of_find_node_by_path]: device node is %s\n", mydev_node->name);

        // 获取父节点
        mydev_node = of_get_parent(mydev_node);
        pr_info("[of_find_node_by_path]: device node is %s\n", mydev_node->name);

        // 获取子节点
        mydev_node = of_get_next_child(mydev_node, NULL);
        pr_info("[of_get_next_child]: device node is %s\n", mydev_node->name);

        // 使用compatible 查找节点
        mydev_node = of_find_compatible_node(NULL, NULL, "my devicetree");
        pr_info("[of_find_compatible_node]: device node is %s\n", mydev_node->name);

        // 使用of_device_id匹配表查找匹配的节点
        mydev_node = of_find_matching_node_and_match(NULL, mynode_of_match, NULL);
        pr_info("[of_find_matching_node_and_match]: device node is %s\n", mydev_node->name);

        return 0;
}

static int my_platform_driver_remove(struct platform_device *pdev)
{
        return 0;
}

static const struct of_device_id match_table[] = { { .compatible = "my devicetree" }, {} };

static struct platform_driver my_platform_driver ={
        .driver = {
                .owner = THIS_MODULE,
                .name = "my_platform_driver",
                .of_match_table = match_table,
        },
        .probe = my_platform_driver_probe,
        .remove = my_platform_driver_remove,
        
};

module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for of api");
