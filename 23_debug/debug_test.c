#include <linux/init.h>
#include <linux/module.h>

static int __init debug_test_init(void)
{
        printk(KERN_INFO "hello world!\n");
        pr_info("------------------------\n");
        dump_stack();
        pr_info("------------------------\n");

        WARN_ON(true);
        pr_info("------------------------\n");

        BUG_ON(true);
        pr_info("------------------------\n");

        panic("test panic");
        
        return 0;
}

static void __exit debug_test_exit(void)
{
        pr_info("hello world module exit\n");
}

module_init(debug_test_init);
module_exit(debug_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("hello world!");
