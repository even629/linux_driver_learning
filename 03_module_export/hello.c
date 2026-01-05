#include <linux/module.h>
#include <linux/init.h>

static int a = MODULE_DEFAULT_A;
module_param(a,int, 0644);
MODULE_PARM_DESC(a, "add test left num a, default 0");

static int b = MODULE_DEFAULT_B;
module_param(b, int, 0644);
MODULE_PARM_DESC(b, "add test left num b, default 0");



extern int add(int a, int b);

static int __init module_export_init(void){
        pr_info("hello init");
	pr_info("a=%d, b=%d\n", a, b);
	pr_info("a+b=%d\n", a+b);
        return 0;
}

static void __exit module_export_exit(void){
        pr_info("hello exit");
}

module_init(module_export_init);
module_exit(module_export_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<731005515@qq.com>");
MODULE_DESCRIPTION("A sample for module export");

