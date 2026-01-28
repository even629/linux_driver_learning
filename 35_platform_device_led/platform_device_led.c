#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

/* 复用寄存器 */
#define PMU_GRF_BASE 0xFDC20000
#define GPIO0B_IOMUX_OFFSET 0xC
#define GPIO0B_IOMUX (PMU_GRF_BASE + GPIO0B_IOMUX_OFFSET)

/* 数据寄存器与方向寄存器 */
#define GPIO0_BASE 0xFDD60000
#define GPIO_SWPORT_DDR_L_OFFSET 0x8
#define GPIO_SWPORT_DR_L_OFFSET 0x0
#define GPIO_EXT_PORT_OFFSET 0x70
// 方向寄存器
#define GPIO0_SWPORT_DDR_L (GPIO0_BASE + GPIO_SWPORT_DDR_L_OFFSET)
// 数据寄存器
#define GPIO0_SWPORT_DR_L (GPIO0_BASE + GPIO_SWPORT_DR_L_OFFSET)
// 外部输入寄存器,readonly
#define GPIO0_EXT_PORT (GPIO0_BASE + GPIO_EXT_PORT_OFFSET)

#define REG_SIZE 4

struct resource my_resources[] = { {
                                           .start = GPIO0B_IOMUX,
                                           .end = GPIO0B_IOMUX + REG_SIZE - 1,
                                           .name = "GPIO0B_IOMUX",
                                           .flags = IORESOURCE_MEM,
                                   },
                                   {
                                           .start = GPIO0_SWPORT_DDR_L,
                                           .end = GPIO0_SWPORT_DDR_L + REG_SIZE - 1,
                                           .name = "GPIO0_SWPORT_DDR_L",
                                           .flags = IORESOURCE_MEM,
                                   },
                                   {
                                           .start = GPIO0_SWPORT_DR_L,
                                           .end = GPIO0_SWPORT_DR_L + REG_SIZE - 1,
                                           .name = "GPIO0_SWPORT_DR_L",
                                           .flags = IORESOURCE_MEM,
                                   },
                                   {
                                           .start = GPIO0_EXT_PORT,
                                           .end = GPIO0_EXT_PORT + REG_SIZE - 1,
                                           .name = "GPIO0_EXT_PORT",
                                           .flags = IORESOURCE_MEM,
                                   } };

static struct platform_device my_platform_device={
        .name = "light_up_led",
        .num_resources = ARRAY_SIZE(my_resources),
        .resource = my_resources,
};

static int __init platform_device_led_init(void)
{
        int ret;
        ret = platform_device_register(&my_platform_device);
        if(ret < 0){
                pr_info("platform_device_register fail\n");
                return ret;
        }
        return 0;
}

static void __exit platform_device_led_exit(void)
{
        platform_device_unregister(&my_platform_device);
}

module_init(platform_device_led_init);
module_exit(platform_device_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is light_up_led example using platform");
