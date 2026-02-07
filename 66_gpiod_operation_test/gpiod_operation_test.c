#include <linux/gpio/driver.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

static struct gpio_desc *mygpiod1;

static int test_pdrv_probe(struct platform_device *pdev)
{
        int ret = 0;
        int direct, value, irq;

        pr_info("%s\n", __func__);

        mygpiod1 = gpiod_get_optional(&pdev->dev, "my", GPIOD_OUT_LOW);
        if (IS_ERR_OR_NULL(mygpiod1)) {
                dev_err(&pdev->dev, "gpiod_get_optional error\n");
                ret = PTR_ERR(mygpiod1);
                goto err_get_gpiod;
        }

        // 设置为高电平
        gpiod_set_value(mygpiod1, 1);
        

        // 获取方向
        direct = gpiod_get_direction(mygpiod1);
        switch (direct) {
        case GPIO_LINE_DIRECTION_IN:
                pr_info("direction is input\n");
                break;
        case GPIO_LINE_DIRECTION_OUT:
                pr_info("direction is output\n");
                break;
        default:
                dev_err(&pdev->dev, "unknown direction\n");
                ret = -EFAULT;
                goto err_op;
        }

        // 读取当前值
        value = gpiod_get_value(mygpiod1);
        pr_info("value is %d\n", value);

        // 获取中断号
        irq = gpiod_to_irq(mygpiod1);
        if (irq < 0) {
                dev_err(&pdev->dev, "get irq error");
                ret = -EFAULT;
                goto err_op;
        }
        pr_info("irq is %d\n", irq);

err_op:
        gpiod_put(mygpiod1);
err_get_gpiod:
        return ret;
}

static int test_pdrv_remove(struct platform_device *pdev)
{
        gpiod_put(mygpiod1);
        return 0;
}

static const struct of_device_id match_table[] = { {
        .compatible = "even629,mygpio",
} };

static struct platform_driver test_pdrv = {
        .driver = {
                .name = "test_gpio",
                .owner = THIS_MODULE,
                .of_match_table = match_table,
        },
        .probe = test_pdrv_probe,
        .remove = test_pdrv_remove,
};

module_platform_driver(test_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com");
MODULE_DESCRIPTION("This is a test sample for gpiod");
