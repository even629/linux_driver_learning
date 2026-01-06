#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>

static int major = 0;

module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "mknod_test dev major num");

static int minor = 0;
module_param(minor, int, S_IRUGO);
MODULE_PARM_DESC(minor, "mknod_test dev minor num");

dev_t dev_num;


int mknod_test_open(struct inode *inode, struct file *file){
        pr_info("This is mknod_test open\n");
        return 0;        
}
ssize_t mknod_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset){
        pr_info("This is mknod test read\n");
        return 0;        
}

int mknod_test_release (struct inode *inode, struct file *file){
        pr_info("This is mknod test release\n");
        return 0;
}
struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = mknod_test_open,
        .read = mknod_test_read,
        .release = mknod_test_release,
};

struct cdev cdev;
struct class *class;
struct device *dev;


static int __init mknod_test_init(void)
{
        int err;

        pr_info("mknod_test module init\n");

        if (major) {
                dev_num = MKDEV(major, minor);
                err = register_chrdev_region(dev_num, 1, "mknod test device num");
                if (err < 0) {
                        pr_err("register_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("register_chrdev_region success\n");
        } else {
                err = alloc_chrdev_region(&dev_num, 0, 1, "mknod test device num");
                if (err < 0) {
                        pr_err("alloc_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("alloc_chrdev_region success\n");
        }
        pr_info("dev_t dev_num: major[%d], minor[%d]\n", MAJOR(dev_num), MINOR(dev_num));

        cdev_init(&cdev, &fops);
        cdev.owner = THIS_MODULE;

        err = cdev_add(&cdev, dev_num, 1);
        if (err < 0) {
                pr_err("cdev add error\n");
                goto cdev_add_err;
        }

        class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(class)) {
                err = PTR_ERR(class);
                pr_err("class_create error\n");
                goto class_create_err;
        }
        dev = device_create(class, NULL, dev_num, NULL, "mknod_test_device");
        if(IS_ERR(dev)){
                err = PTR_ERR(dev);
                pr_err("device create error\n");
                goto device_create_err;
        }
       
        return 0;
 device_create_err:
        class_destroy(class);
class_create_err:
        cdev_del(&cdev);
cdev_add_err:
        unregister_chrdev_region(dev_num, 1);
chrdev_region_err:
        return err;
}

static void __exit mknod_test_exit(void)
{
        device_destroy(class, dev_num);
        class_destroy(class);
        cdev_del(&cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_info("mknod_test module exit\n");
}
module_init(mknod_test_init);
module_exit(mknod_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is just mknod test sample");
