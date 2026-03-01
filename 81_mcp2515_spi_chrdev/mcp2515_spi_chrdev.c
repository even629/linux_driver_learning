#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

int mcp2515_probe(struct spi_device *spi){
        pr_info("%s\n", __func__);
        return 0;
}
int mcp2515_remove(struct spi_device *spi){
        return 0;
}

static const struct of_device_id mcp2515_match_table[] = {
        {.compatible = "my-mcp2515"},
        {}
};

MODULE_DEVICE_TABLE(of, mcp2515_match_table);

static const struct spi_device_id mcp2515_id_table[] = {
        {.name = "mcp2515"},
        {}
};

MODULE_DEVICE_TABLE(spi, mcp2515_id_table);

static struct spi_driver spi_mcp2515 = {
        .probe = mcp2515_probe,
        .remove = mcp2515_remove,
        .driver = {
                .name = "mcp2515",
                .owner = THIS_MODULE,
                .of_match_table = mcp2515_match_table,
        },
        .id_table = mcp2515_id_table,
};

module_spi_driver(spi_mcp2515);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629 <asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is test sample for mcp2515 spi driver");
