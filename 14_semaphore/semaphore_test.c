#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/sched.h>

struct test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
};

static struct test_drv_data *drv_dat;
static struct semaphore sema;

int semaphore_test_open(struct inode *inode, struct file *file)
{
        int ret;
        file->private_data = drv_dat;
        ret = down_interruptible(&sema);
        if (ret != 0) {
                pr_info("semaphore_test_open is called by [pid: %d] but is interrupted\n",
                        current->pid);
                return -EINTR;
        }
        pr_info("semaphore_test_open is called by [pid: %d]\n", current->pid);
        return 0;
}

int semaphore_test_release(struct inode *inode, struct file *file)
{
        up(&sema);
        pr_info("semaphore_test_release is called by [pid: %d]\n", current->pid);
        return 0;
}
struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = semaphore_test_open,
        .release = semaphore_test_release,
};

static int __init semaphore_test_init(void)
{
        int err;

        drv_dat = (struct test_drv_data *)kzalloc(sizeof(struct test_drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                err = -ENOMEM;
                goto kzalloc_fail;
        }

        err = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "semaphore_test_chrdev_region");
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
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "semaphore_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                err = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        // 初始化信号量
        sema_init(&sema, 1);

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

static void __exit semaphore_test_exit(void)
{
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(semaphore_test_init);
module_exit(semaphore_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is test sample for semaphore");
