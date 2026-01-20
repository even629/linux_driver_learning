#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

struct drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
};

static struct drv_data *drv_dat;

static void timer_irq_func(struct timer_list *t);

DEFINE_TIMER(timer_test, timer_irq_func);

int timer_mod_test_open(struct inode *inode, struct file *file)
{
        file->private_data = drv_dat;
        pr_info("open is called by pid: %d\n", task_pid_nr(current));
        return 0;
}

ssize_t timer_mod_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        size_t len = min(sizeof(u64) + 1, size);
        char kbuf[72];

        snprintf(kbuf, sizeof(kbuf), "%llu", jiffies64_to_msecs(jiffies_64));

        if (copy_to_user(buf, kbuf, len) != 0)
                return -EFAULT;

        pr_info("read is called by pid: %d\n", task_pid_nr(current));
        return len;
}

int timer_mod_test_release(struct inode *inode, struct file *file)
{
        pr_info("release is called by pid: %d\n", task_pid_nr(current));
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = timer_mod_test_open,
        .read = timer_mod_test_read,
        .release = timer_mod_test_release,
};

static void timer_irq_func(struct timer_list *t)
{
        pr_info("timer_irq_func is called\n");
        mod_timer(&timer_test, jiffies_64 + msecs_to_jiffies(3000));
}

static int __init timer_mod_test_init(void)
{
        int ret;
        drv_dat = kzalloc(sizeof(struct drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }
        ret = alloc_chrdev_region(&drv_dat->dev_num, 1, 0, "timer_mod_test_chrdev_region");
        if (ret < 0)
                goto alloc_chrdev_region_fail;

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
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "timer_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                ret = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        // 把timer_test设置为5s后
        timer_test.expires = jiffies_64 + msecs_to_jiffies(3000);
        // 添加一个定时器
        add_timer(&timer_test);

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
        return ret;
}

static void __exit timer_mod_test_exit(void)
{
        // 删除定时器
        del_timer(&timer_test);
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);

        cdev_del(&drv_dat->cdev);

        unregister_chrdev_region(drv_dat->dev_num, 1);

        kfree(drv_dat);
}

module_init(timer_mod_test_init);
module_exit(timer_mod_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for linux timer");
