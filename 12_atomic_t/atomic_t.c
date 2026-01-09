#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/atomic.h>

struct test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
};

static struct test_drv_data *test_drv_data;

static atomic_t atomic_key = ATOMIC_INIT(1);

int atomic_t_test_open(struct inode *inode, struct file *file)
{
        file->private_data = test_drv_data;
        if (atomic_dec_and_test(&atomic_key) == 0){
                pr_err("this device is opened by another process\n");
                return -EBUSY;
        }                
        pr_info("atomic_t_test_open is called\n");
        return 0;
}

int atomic_t_test_release(struct inode *inode, struct file *file)
{
        atomic_inc(&atomic_key);
        return 0;
}

ssize_t atomic_t_test_read (struct file *file, char __user *buf, size_t size, loff_t *offset){
	pr_info("atomic_t_test_read is called\n");
	return 0;
}
ssize_t atomic_t_test_write (struct file *file, const char __user *buf, size_t size, loff_t *offset){
	pr_info("atomic_t_test_write is called\n");
	return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = atomic_t_test_open,
        .release = atomic_t_test_release,
};

static int __init atomic_t_test_init(void)
{
        int err;
        test_drv_data = (struct test_drv_data *)kzalloc(sizeof(struct test_drv_data), GFP_KERNEL);
        if (test_drv_data == NULL) {
                err = -ENOMEM;
                goto kzalloc_fail;
        }
        err = alloc_chrdev_region(&test_drv_data->dev_num, 0, 1, "atomic_t test chrdev region");
        if (err < 0)
                goto alloc_chrdev_region_fail;
        cdev_init(&test_drv_data->cdev, &fops);
        err = cdev_add(&test_drv_data->cdev, test_drv_data->dev_num, 1);
        if (err < 0)
                goto cdev_add_fail;
        test_drv_data->class = class_create(THIS_MODULE, "atomic_t_test");
        if (IS_ERR(test_drv_data->class)) {
                err = PTR_ERR(test_drv_data->class);
                goto class_create_fail;
        }
        test_drv_data->dev = device_create(test_drv_data->class, NULL, test_drv_data->dev_num, NULL,
                                           "atomic_t_test%d", 0);
        if (IS_ERR(test_drv_data->dev)) {
                err = PTR_ERR(test_drv_data->dev);
                goto device_create_fail;
        }

        return 0;
device_create_fail:
        class_destroy(test_drv_data->class);
class_create_fail:
        cdev_del(&test_drv_data->cdev);
cdev_add_fail:
        unregister_chrdev_region(test_drv_data->dev_num, 1);
alloc_chrdev_region_fail:
        kfree(test_drv_data);
kzalloc_fail:
        return err;
}

static void __exit atomic_t_test_exit(void)
{
        device_destroy(test_drv_data->class, test_drv_data->dev_num);
        class_destroy(test_drv_data->class);
        cdev_del(&test_drv_data->cdev);
        unregister_chrdev_region(test_drv_data->dev_num, 1);
        kfree(test_drv_data);
}

module_init(atomic_t_test_init);
module_exit(atomic_t_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for atomic_t");
