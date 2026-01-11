#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/sched.h>

struct test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
};

static struct test_drv_data *drv_dat;
static struct mutex drv_mutex;

int mutex_test_open(struct inode *inode, struct file *file)
{
        int err;
        file->private_data = drv_dat;
        err = mutex_lock_interruptible(&drv_mutex);
        if (err != 0) {
                pr_err("pid: %d is interrupted", current->pid);
                return err;
        }
        pr_info("mutex_test_open is called by pid: %d\n", current->pid);
        return 0;
}

int mutex_test_release(struct inode *inode, struct file *file)
{
        mutex_unlock(&drv_mutex);
        pr_info("mutex_test_release is called by pid: %d\n", current->pid);
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = mutex_test_open,
        .release = mutex_test_release,
};

static int __init mutex_test_init(void)
{
        int err;
        drv_dat = (struct test_drv_data *)kzalloc(sizeof(struct test_drv_data), GFP_KERNEL);

        if (drv_dat == NULL) {
                err = -ENOMEM;
                goto kzalloc_fail;
        }

        err = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "mutex_test_chrdev_region");
        if (err < 0)
                goto alloc_chrdev_region_fail;

        cdev_init(&drv_dat->cdev, &fops);
        drv_dat->cdev.owner = THIS_MODULE;
        err = cdev_add(&drv_dat->cdev, drv_dat->dev_num, 1);
        if (err < 0)
                goto cdev_add_fail;

        drv_dat->class = class_create(THIS_MODULE, "chrdev_test");
        if (IS_ERR(drv_dat->class)) {
                err = PTR_ERR(drv_dat->class);
                goto class_create_fail;
        }

        drv_dat->dev =
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "mutex_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                err = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        // 初始化互斥锁
        mutex_init(&drv_mutex);
        return 0;
device_create_fail:
        class_destroy(drv_dat->class);
class_create_fail:
        cdev_del(&drv_dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(drv_dat->dev_num, 1);
alloc_chrdev_region_fail:
        kfree(drv_dat);
kzalloc_fail:
        return err;
}

static void __exit mutex_test_exit(void)
{
        kfree(drv_dat);
}

module_init(mutex_test_init);
module_exit(mutex_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for mutex");
