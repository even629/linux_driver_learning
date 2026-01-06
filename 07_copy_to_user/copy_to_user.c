#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cdev.h>

static int major = 0;
module_param(major, int, S_IRUGO);
MODULE_PARM_DESC(major, "copy_to_user test device num: major");

static int minor = 0;
module_param(minor, int, S_IRUGO);
MODULE_PARM_DESC(major, "copy_to_user test device num: minor");

int cpy2usr_test_open(struct inode *inode, struct file *file)
{
        pr_info("cpy2usr_test_open was called\n");
        return 0;
}

ssize_t cpy2usr_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        char kbuf[] = "Hello World!\n";

        pr_info("cpy2user_test_read was called\n");

        if (copy_to_user(buf, kbuf, strlen(kbuf)) != 0) {
                pr_err("copy_to_user error\n");
                return -ENODATA;
        }

        return strlen(kbuf);
}

ssize_t cpy2usr_test_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        char kbuf[32];

        pr_info("cpy2user_test_write was called\n");
        
        if (size > 32) {
                size = 32;
        }
        
        if (copy_from_user(kbuf, buf, size)) {
                pr_err("copy_from_user error\n");
                return -ENOMEM;
        }
        pr_info("read from user buf: %s", kbuf);
        return size;
}

int cpy2usr_test_release(struct inode *inode, struct file *file)
{
        pr_info("cpy2user_test_release was called\n");
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = cpy2usr_test_open,
        .read = cpy2usr_test_read,
        .write = cpy2usr_test_write,
        .release = cpy2usr_test_release,
};

struct copy2user_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *device;
};

struct copy2user_data *cpy2usr_dat;

static int __init copy2user_test_init(void)
{
        int err;

        pr_info("copy_to_user test init\n");

        cpy2usr_dat = (struct copy2user_data *)kzalloc(sizeof(struct copy2user_data), GFP_KERNEL);

        if (major) {
                cpy2usr_dat->dev_num = MKDEV(major, minor);
                err = register_chrdev_region(cpy2usr_dat->dev_num, 1,
                                             "copy_to_user test device num");
                if (err < 0) {
                        pr_err("register_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("register_chrdev_region success\n");
        } else {
                err = alloc_chrdev_region(&cpy2usr_dat->dev_num, 0, 1,
                                          "copy_to_user test device num");
                if (err < 0) {
                        pr_err("alloc_chrdev_region error\n");
                        goto chrdev_region_err;
                }
                pr_info("alloc_chrdev_region success\n");
        }

        cdev_init(&cpy2usr_dat->cdev, &fops);
        cpy2usr_dat->cdev.owner = THIS_MODULE;
        err = cdev_add(&cpy2usr_dat->cdev, cpy2usr_dat->dev_num, 1);

        if (err < 0) {
                pr_err("cdev_add error\n");
                goto cdev_add_err;
        }
        cpy2usr_dat->class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(cpy2usr_dat->class)) {
                err = PTR_ERR(cpy2usr_dat->class);
                goto class_create_err;
        }
        cpy2usr_dat->device = device_create(cpy2usr_dat->class, NULL, cpy2usr_dat->dev_num, NULL,
                                            "cpy2usr_test_dev");
        if (IS_ERR(cpy2usr_dat->device)) {
                err = PTR_ERR(cpy2usr_dat->device);
                goto device_create_err;
        }

        return 0;
device_create_err:
        class_destroy(cpy2usr_dat->class);
class_create_err:
        cdev_del(&cpy2usr_dat->cdev);
cdev_add_err:
        unregister_chrdev_region(cpy2usr_dat->dev_num, 1);
chrdev_region_err:
        kfree(cpy2usr_dat);
        return err;
}

static void __exit copy2user_test_exit(void)
{
        device_destroy(cpy2usr_dat->class, cpy2usr_dat->dev_num);
        class_destroy(cpy2usr_dat->class);
        cdev_del(&cpy2usr_dat->cdev);
        unregister_chrdev_region(cpy2usr_dat->dev_num, 1);
        kfree(cpy2usr_dat);
        pr_info("copy_to_user test exit\n");
}

module_init(copy2user_test_init);
module_exit(copy2user_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is just a copy_to_user test sample");
