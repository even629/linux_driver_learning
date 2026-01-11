#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

struct test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;        
};

static struct test_drv_data *test_drv_dat;
static bool status = 1;
static spinlock_t lock;

int spinlock_test_open(struct inode *inode, struct file *file)
{
        file->private_data = test_drv_dat;
        spin_lock(&lock);
        if(status == 0){
                spin_unlock(&lock);
                return -EBUSY;
        }
        status = 0;   
        spin_unlock(&lock);
        pr_info("spinlock_test_open is called by [pid: %d]\n",task_pid_nr(current));
        
        return 0;
}
ssize_t spinlock_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        pr_info("spinlock_test_read is called\n");
        return 0;
}
ssize_t spinlock_test_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        pr_info("spinlock_test_write is called\n");
        return 0;
}
int spinlock_test_release(struct inode *inode, struct file *file)
{
        spin_lock(&lock);        
        status = 1;        
        spin_unlock(&lock);
        pr_info("spinlock_test_release is called by [pid: %d]\n", task_pid_nr(current));
        return 0;
}
static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = spinlock_test_open,
        .read = spinlock_test_read,
        .write = spinlock_test_write,
        .release = spinlock_test_release,
};

static int __init spinlock_test_init(void)
{
        int err;
        test_drv_dat = kzalloc(sizeof(struct test_drv_data), GFP_KERNEL);
        if (test_drv_dat == NULL) {
                err = -ENOMEM;
                goto kzalloc_fail;
        }
        err = alloc_chrdev_region(&test_drv_dat->dev_num, 0, 1, "spinlock test chrdev region\n");

        if (err < 0)
                goto alloc_chrdev_region_fail;
        cdev_init(&test_drv_dat->cdev, &fops);
        test_drv_dat->cdev.owner = THIS_MODULE;
        err = cdev_add(&test_drv_dat->cdev, test_drv_dat->dev_num, 1);
        if (err < 0)
                goto cdev_add_fail;
        test_drv_dat->class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(test_drv_dat->class)) {
                err = PTR_ERR(test_drv_dat->class);
                goto class_create_fail;
        }
        test_drv_dat->dev = device_create(test_drv_dat->class, NULL, test_drv_dat->dev_num, NULL,
                                          "spinlock_test%d", 0);
        if (IS_ERR(test_drv_dat->dev)) {
                err = PTR_ERR(test_drv_dat->dev);
                goto device_create_fail;
        }
        // 初始化自旋锁
        spin_lock_init(&lock);

        return 0;

device_create_fail:
        class_destroy(test_drv_dat->class);
class_create_fail:
        cdev_del(&test_drv_dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(test_drv_dat->dev_num, 1);
alloc_chrdev_region_fail:
        kfree(test_drv_dat);
kzalloc_fail:
        return err;
}

static void __exit spinlock_test_exit(void)
{
        device_destroy(test_drv_dat->class, test_drv_dat->dev_num);
        class_destroy(test_drv_dat->class);
        cdev_del(&test_drv_dat->cdev);
        unregister_chrdev_region(test_drv_dat->dev_num, 1);
        kfree(test_drv_dat);
}

module_init(spinlock_test_init);
module_exit(spinlock_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for spinlock");
