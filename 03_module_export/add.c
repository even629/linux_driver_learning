#include <linux/module.h>
#include <linux/init.h>

int add(int a, int b){
        return a+b;
}
EXPORT_SYMBOL(add);

static int __init module_export_init(void){
	pr_info("add init\n");
	return 0;
}

static void __exit module_export_exit(void){
	pr_info("add exit\n");
}

module_init(module_export_init);
module_exit(module_export_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("A sample for module export");
