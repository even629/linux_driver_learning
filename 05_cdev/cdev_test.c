#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>

static int major = 0;
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "cdev_test sample, major device number");

static int minor = 0;
module_param(minor, int, S_IRUGO);
MODULE_PARM_DESC(minor, "cdev_test sample, minor device number");

dev_t dev_num;
struct cdev cdev;

int cdev_test_open(struct inode *inode, struct file *file){
	pr_info("cdev_test open");
	return 0;
}


ssize_t cdev_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset){
	pr_info("cdev_test read\n");
	return 0;
}

ssize_t cdev_test_write(struct file *file, const char __user *buf, size_t, loff_t *offset){
	pr_info("cdev_test write\n");
	return 0;
}


struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = cdev_test_open,
	.read = cdev_test_read,
	.write = cdev_test_write,
};

static int __init cdev_test_init(void)
{
        int err;
        if (major) {
                dev_num = MKDEV(major, minor);
                err = register_chrdev_region(dev_num, 1, "cdev test dev num");
                if (err < 0) {
                        pr_err("register_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("register_chrdev_region success\n");
        } else {
                err = alloc_chrdev_region(&dev_num, 0, 1, "cdev test dev num");
                if (err < 0) {
                        pr_err("alloc_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("alloc_chrdev_region success\n");
        }
	pr_info("dev_num: major[%d] minor[%d]", MAJOR(dev_num), MINOR(dev_num));
	
        cdev_init(&cdev, &fops);
        pr_info("cdev_init success\n");
        cdev.owner = THIS_MODULE; // 将owner指向本模块，防止cdev操作时卸载模块

        err = cdev_add(&cdev, dev_num, 1);
        pr_info("cdev_add success\n");
        if (err < 0) {
                pr_err("cdev_add error\n");
                goto cdev_add_err;
        }

        return 0;
cdev_add_err:
        unregister_chrdev_region(dev_num, 1);
chrdev_region_err:
        return err;
}

static void __exit cdev_test_exit(void)
{
        cdev_del(&cdev);
        pr_info("cdev is deleted\n");
        unregister_chrdev_region(dev_num, 1);
        pr_info("chrdev_region unregister success\n");
}

module_init(cdev_test_init);
module_exit(cdev_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is just a cdev test sample");
