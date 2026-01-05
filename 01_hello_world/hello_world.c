#include <linux/init.h>
#include <linux/module.h>

static int __init hello_world_init(void){
        printk(KERN_INFO "hello world!\n");
        return 0;
}

static void __exit hello_world_exit(void){
        pr_info("hello world module exit\n");
}
module_init(hello_world_init);
module_exit(hello_world_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("hello world!");
