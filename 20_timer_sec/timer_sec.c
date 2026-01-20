#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/string.h>

struct timer_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        atomic64_t sec;
};

static struct timer_drv_data *timer_drv_data;

static void timer_sec_func(struct timer_list *t);

DEFINE_TIMER(timer_sec, timer_sec_func);

int timer_sec_test_open(struct inode *inode, struct file *file)
{
        file->private_data = timer_drv_data;
        // 初始化为0
        atomic64_set(&timer_drv_data->sec, 0);
        add_timer(&timer_sec);

        pr_info("open is called by pid: %d\n", task_pid_nr(current));
        return 0;
}

ssize_t timer_sec_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct timer_drv_data *dat = file->private_data;
        size_t len;

        char kbuf[128];
        snprintf(kbuf, sizeof(kbuf), "current sec: %llu\n", atomic64_read(&dat->sec));

        len = min(size, strlen(kbuf) + 1);
        if (copy_to_user(buf, kbuf, len) != 0)
                return -EFAULT;

        return len;
}

int timer_sec_test_release(struct inode *inode, struct file *file)
{
        del_timer(&timer_sec);
        pr_info("release is called by pid: %d\n", task_pid_nr(current));
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = timer_sec_test_open,
        .read = timer_sec_test_read,
        .release = timer_sec_test_release,
};

static void timer_sec_func(struct timer_list *t)
{
        atomic64_inc(&timer_drv_data->sec);
        mod_timer(&timer_sec, get_jiffies_64() + msecs_to_jiffies(1000));
}

static int __init timer_sec_init(void)
{
        int ret;
        timer_drv_data = kzalloc(sizeof(struct timer_drv_data), GFP_KERNEL);
        if (timer_drv_data == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }

        ret = alloc_chrdev_region(&timer_drv_data->dev_num, 0, 1, "test_chrdev_region");
        if (ret < 0)
                goto alloc_chrdev_region_fail;

        cdev_init(&timer_drv_data->cdev, &fops);
        timer_drv_data->cdev.owner = THIS_MODULE;
        ret = cdev_add(&timer_drv_data->cdev, timer_drv_data->dev_num, 1);
        if (ret < 0)
                goto cdev_add_fail;

        timer_drv_data->class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(timer_drv_data->class)) {
                ret = PTR_ERR(timer_drv_data->class);
                goto class_create_fail;
        }

        timer_drv_data->dev = device_create(timer_drv_data->class, NULL, timer_drv_data->dev_num,
                                            NULL, "timer_sec%d", 0);
        if (IS_ERR(timer_drv_data->dev)) {
                ret = PTR_ERR(timer_drv_data->dev);
                goto device_create_fail;
        }

        return 0;
device_create_fail:
        class_destroy(timer_drv_data->class);
class_create_fail:
        cdev_del(&timer_drv_data->cdev);
cdev_add_fail:
        unregister_chrdev_region(timer_drv_data->dev_num, 1);
alloc_chrdev_region_fail:
        kfree(timer_drv_data);
kzalloc_fail:
        return ret;
}

static void __exit timer_sec_exit(void)
{
        class_destroy(timer_drv_data->class);

        cdev_del(&timer_drv_data->cdev);

        unregister_chrdev_region(timer_drv_data->dev_num, 1);

        kfree(timer_drv_data);
}

module_init(timer_sec_init);
module_exit(timer_sec_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a sec timer");
