#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

#define KMEM_SIZE 32
struct test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        struct mutex lock;
        char kmem[KMEM_SIZE];
};

static struct test_drv_data *drv_dat;

int test_open(struct inode *inode, struct file *file)
{
        file->private_data = drv_dat;
        return 0;
}

ssize_t test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        int ret;
        struct test_drv_data *dat = file->private_data;
        size_t len;

        if (*offset >= KMEM_SIZE)
                return 0; // EOF
        len = min(size, (size_t)(KMEM_SIZE - *offset));

        ret = mutex_lock_interruptible(&dat->lock);
        if (ret < 0) {
                pr_info("test_read is interrupted while acquiring the mutex\n");
                return ret;
        }
        if (copy_to_user(buf, dat->kmem + *offset, len) != 0) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }

        mutex_unlock(&dat->lock);

        *offset += len;
        return len;
}

ssize_t test_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        int ret;
        struct test_drv_data *dat = file->private_data;
        size_t len;

        if (*offset >= KMEM_SIZE)
                return 0; // EOF

        len = min(size, (size_t)(KMEM_SIZE - *offset));

        ret = mutex_lock_interruptible(&dat->lock);
        if (ret < 0) {
                pr_info("test_read is interrupted while acquiring the mutex\n");
                return ret;
        }
        if (copy_from_user(dat->kmem + *offset, buf, len) != 0) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }

        mutex_unlock(&dat->lock);
        *offset += len;
        return len;
}

loff_t test_llseek(struct file *file, loff_t offset, int whence)
{
        return fixed_size_llseek(file, offset, whence, KMEM_SIZE);
}

int test_release(struct inode *inode, struct file *file)
{
        return 0;
}

#define CMD_CLEAR _IO('L', 0)

long test_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        struct test_drv_data *dat = file->private_data;
        int ret;
        switch (cmd) {
        case CMD_CLEAR:
                ret = mutex_lock_interruptible(&dat->lock);
                if (ret < 0) {
                        pr_info("test_unlocked_ioctl is interrupted while acquiring the mutex\n");
                        return ret;
                }
                memset(dat->kmem, 0, sizeof(dat->kmem));
                mutex_unlock(&dat->lock);
                file->f_pos = 0;
                break;
        default:
                return -ENOTTY;
        }
        return 0;
}
struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = test_open,
        .read = test_read,
        .write = test_write,
        .llseek = test_llseek,
        .unlocked_ioctl = test_unlocked_ioctl,
        .release = test_release,
};

static int __init ioctl_test_init(void)
{
        int ret;
        drv_dat = kzalloc(sizeof(struct test_drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }
        ret = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "test_chrdev_region");
        if (ret < 0)
                goto alloc_chrdev_region;
        cdev_init(&drv_dat->cdev, &fops);
        drv_dat->cdev.owner = THIS_MODULE;
        ret = cdev_add(&drv_dat->cdev, drv_dat->dev_num, 1);
        if (ret < 0)
                goto cdev_add_fail;

        drv_dat->class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(drv_dat->class)) {
                ret = PTR_ERR(drv_dat->class);
                goto class_create_fail;
        }
        drv_dat->dev =
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "ioctl_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                ret = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        mutex_init(&drv_dat->lock);

        return 0;
device_create_fail:
        class_destroy(drv_dat->class);
class_create_fail:
        cdev_del(&drv_dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(drv_dat->dev_num, 1);
alloc_chrdev_region:
        kfree(drv_dat);
kzalloc_fail:
        return ret;
}

static void __exit ioctl_test_exit(void)
{
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(ioctl_test_init);
module_exit(ioctl_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test description for ioctl");
