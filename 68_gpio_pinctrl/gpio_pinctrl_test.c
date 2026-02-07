#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/pinctrl/pinctrl.h>

static struct pinctrl *pinctrl;

int test_pdrv_probe(struct platform_device *pdev)
{
        int ret = 0;
        struct device *dev = &pdev->dev;
        struct pinctrl_state *default_stat, *sleep_stat;

        // 1. 获取pinctrl
        pinctrl = pinctrl_get(dev);
        if (IS_ERR(pinctrl)) {
                dev_err(dev, "Failed to get pinctrl");
                return PTR_ERR(pinctrl);
        }

        // 2. 查找状态
        default_stat = pinctrl_lookup_state(pinctrl, "default");
        if (IS_ERR(pinctrl)) {
                dev_err(dev, "Failed to lookup 'default' state\n");
                ret = PTR_ERR(default_stat);
                goto err;
        }

        sleep_stat = pinctrl_lookup_state(pinctrl, "sleep");
        if (IS_ERR(pinctrl)) {
                dev_err(dev, "Failed to lookup 'sleep' state\n");
                ret = PTR_ERR(sleep_stat);
                goto err;
        }

        // 3. 应用状态
        ret = pinctrl_select_state(pinctrl, sleep_stat);
        if (ret < 0) {
                dev_err(dev, "Failed to select 'sleep' state\n");
                ret = -EFAULT;
                goto err;
        }

        return ret;

err:
        pinctrl_put(pinctrl);
        return ret;
}
int test_pdrv_remove(struct platform_device *pdev)
{
        pinctrl_put(pinctrl);
        return 0;
}

const struct of_device_id match_table[] = {
        { .compatible = "even629,test-device" },
};

static struct platform_driver test_pdrv = {
        .driver = {
                .name = "test-gpio-pinctrl",
                .owner = THIS_MODULE,
                .of_match_table = match_table,
        },
        .probe = test_pdrv_probe,
        .remove = test_pdrv_remove,
};

module_platform_driver(test_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com");
MODULE_DESCRIPTION("This is test sample");
