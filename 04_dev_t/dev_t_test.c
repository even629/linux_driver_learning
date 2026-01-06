#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/err.h>


static int major = 0;
module_param(major, int, S_IRUGO); // read only
MODULE_PARM_DESC(major, "custom dev major num");

static int minor = 0;
module_param(minor, int, S_IRUGO); // read only
MODULE_PARM_DESC(minor, "custom dev minor num");

static dev_t dev_num;

static int __init dev_t_test_init(void)
{
        int err;
        pr_info("dev_t test init");

        if (major) {
                dev_num = MKDEV(major, minor);
                err = register_chrdev_region(dev_num, 1, "dev_t_test device");
                if (err < 0) {
			pr_err("register_chrdev_region error\n");
			return err;
                }
		pr_info("register_chrdev_region success\n");

        } else {
		err = alloc_chrdev_region(&dev_num, 0, 1, "dev_t_test device");
		if(err < 0){
			pr_err("alloc_chrdev_region error\n");
			return err;
		}
		pr_info("alloc_chrdev_region success\n");
        }

        return 0;
}

static void __exit dev_t_test_exit(void)
{
	unregister_chrdev_region(dev_num, 1);
        pr_info("dev_t test module exit\n");
}

module_init(dev_t_test_init);
module_exit(dev_t_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This module is dev_t test sample");
