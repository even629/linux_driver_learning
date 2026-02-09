#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/slab.h>
struct mygpio_data {
        struct pinctrl *gpio_pinctrl;
        struct pinctrl_state *func1_state;
        struct pinctrl_state *func2_state;
};

ssize_t func_state_attr_store(struct device *dev, struct device_attribute *attr, const char *buf,
                              size_t count)
{
        unsigned long func_state_option;
        struct mygpio_data *data = dev_get_drvdata(dev);
        int ret = 0;

        ret = kstrtoul(buf, 10, &func_state_option);
        if (ret)
                return ret;        

        switch (func_state_option) {
        case 0:
                ret = pinctrl_select_state(data->gpio_pinctrl,
                                           data->func1_state);
                break;
        case 1:
                ret = pinctrl_select_state(data->gpio_pinctrl,
                                           data->func2_state);
                break;
        default:
                return -EINVAL;
        }

        if(ret)
                return ret;

        return count;
}

const struct device_attribute func_state_attr = {
        .attr = { .name = "selectmux", .mode = S_IWUSR },
        .show = NULL,
        .store = func_state_attr_store,
};

int test_pdrv_probe(struct platform_device *pdev)
{
        int ret = 0;
        struct device *dev = &pdev->dev;
        struct mygpio_data *mygpio_dat;

        mygpio_dat = devm_kzalloc(dev, sizeof(*mygpio_dat), GFP_KERNEL);
        if (mygpio_dat == NULL)
                return -ENOMEM;

        platform_set_drvdata(pdev, mygpio_dat);

        mygpio_dat->gpio_pinctrl = devm_pinctrl_get(dev);

        if (IS_ERR(mygpio_dat->gpio_pinctrl)) {
                dev_err(dev, "pinctrl_get error\n");
                return PTR_ERR(mygpio_dat->gpio_pinctrl);
        }

        mygpio_dat->func1_state = pinctrl_lookup_state(mygpio_dat->gpio_pinctrl, "mygpio_func1");
        if (IS_ERR(mygpio_dat->func1_state)) {
                dev_err(dev, "pinctrl_lookup_state mygpio_func1 errror\n");
                return PTR_ERR(mygpio_dat->func1_state);
        }

        mygpio_dat->func2_state = pinctrl_lookup_state(mygpio_dat->gpio_pinctrl, "mygpio_func2");
        if (IS_ERR(mygpio_dat->func2_state)) {
                dev_err(dev, "pinctrl_lookup_state mygpio_func2 errror\n");
                return PTR_ERR(mygpio_dat->func2_state);
        }
        device_create_file(dev, &func_state_attr);

        return ret;
}

int test_pdrv_remove(struct platform_device *pdev)
{
        struct device *dev = &pdev->dev;
        device_remove_file(dev, &func_state_attr);
        return 0;
}

const struct of_device_id match_table[] = {
        { .compatible = "even629,mygpio" },
};

struct platform_driver test_pdrv = {
        .driver = {
                .name = "test-pdrv",
                .owner = THIS_MODULE,
                .of_match_table = match_table,
        },
        .probe = test_pdrv_probe,
        .remove = test_pdrv_remove,
};

module_platform_driver(test_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for dynamic change pinmux");
