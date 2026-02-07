#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>

struct gpio_desc *mygpiod1;
struct gpio_desc *mygpiod2;


static int gpiod_test_pdrv_probe(struct platform_device *pdev)
{
        int ret = 0;
        int gpio_num;
        
        pr_info("%s\n", __func__);

        mygpiod1 = gpiod_get_optional(&pdev->dev, "my", GPIOD_IN);
        if (IS_ERR_OR_NULL(mygpiod1)) {
                dev_err(&pdev->dev, "gpiod_get_optional failed: errno %ld\n", PTR_ERR(mygpiod1));
                ret = PTR_ERR(mygpiod1);
                goto err_get_mygpiod;
        }

        gpio_num = desc_to_gpio(mygpiod1);
        pr_info("get gpio num: %d\n", gpio_num);

        gpiod_put(mygpiod1);

        mygpiod2 = gpiod_get_index_optional(&pdev->dev, "my", 0, GPIOD_IN);
        if (IS_ERR_OR_NULL(mygpiod2)) {
                dev_err(&pdev->dev, "gpiod_get_index_optional failed: errno %ld\n",
                        PTR_ERR(mygpiod1));
                ret = PTR_ERR(mygpiod1);
                goto err_get_mygpiod;
        }

        gpio_num = desc_to_gpio(mygpiod2);
        pr_info("get gpio num: %d\n", gpio_num);

        gpiod_put(mygpiod2);

err_get_mygpiod:
        return ret;
}
static int gpiod_test_pdrv_remove(struct platform_device *pdev)
{
        pr_info("%s\n", __func__);
        return 0;
}

const struct of_device_id match_table_id[] = {
        { .compatible = "even629,mygpio" },
};

static struct platform_driver gpiod_test_pdrv = {
        .driver = {
                .name = "test_gpiod_test",
                .owner = THIS_MODULE,
                .of_match_table = match_table_id,    
        },
        .probe = gpiod_test_pdrv_probe,
        .remove = gpiod_test_pdrv_remove,
        
};

module_platform_driver(gpiod_test_pdrv);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for gpiod");
