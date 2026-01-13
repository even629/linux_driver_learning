#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/mutex.h>

#define KBUF_SIZE 32
struct poll_test_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        char kbuf[KBUF_SIZE];
        wait_queue_head_t waitque;
        struct mutex lock;
};

static struct poll_test_drv_data *drv_dat;

int poll_test_open(struct inode *inode, struct file *file)
{
        file->private_data = drv_dat;
        pr_info("open is called by pid: %d\n", current->pid);
        return 0;
}
ssize_t poll_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        int ret;
        size_t len;
        struct poll_test_drv_data *dat = file->private_data;

        pr_info("read is called by pid: %d\n", current->pid);

        if (file->f_flags & O_NONBLOCK) {
                mutex_lock(&dat->lock);
                if (dat->kbuf[0] == '\0') {
                        mutex_unlock(&dat->lock);
                        return -EAGAIN;
                }
                mutex_unlock(&dat->lock);
        } else {
                /* wait_event 条件必须“可重入、可重复检查” */
                ret = wait_event_interruptible(dat->waitque, ({
                                                       int ready;
                                                       mutex_lock(&dat->lock);
                                                       ready = (dat->kbuf[0] != '\0');
                                                       mutex_unlock(&dat->lock);
                                                       ready;
                                               }));

                if (ret < 0) {
                        pr_info("pid: %d read is interrupted while waiting\n", current->pid);
                        return ret;
                }
        }

        mutex_lock(&dat->lock);

        len = min(size, strlen(dat->kbuf) + 1);

        if (copy_to_user(buf, dat->kbuf, len)) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }

        dat->kbuf[0] = '\0'; // reset kbuf string

        mutex_unlock(&dat->lock);

        return len;
}
ssize_t poll_test_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        size_t len;
        struct poll_test_drv_data *dat = file->private_data;

        pr_info("write is called by pid: %d\n", current->pid);

        len = min(size, (size_t)(KBUF_SIZE - 1));

        mutex_lock(&dat->lock);

        if (copy_from_user(dat->kbuf, buf, len)) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }

        dat->kbuf[len] = '\0'; // make sure kbuf a valid string(end with '\0')

        mutex_unlock(&dat->lock);

        /* 在解锁后唤醒 */
        wake_up_interruptible(&dat->waitque);

        return len;
}

__poll_t poll_test_poll(struct file *file, struct poll_table_struct *p)
{
        struct poll_test_drv_data *dat = file->private_data;
        __poll_t mask = 0;

        
        pr_info("poll is called by pid: %d\n", current->pid);
        poll_wait(file, &dat->waitque, p);        

        mutex_lock(&dat->lock);
        if (dat->kbuf[0] != '\0')
                mask |= POLLIN | POLLRDNORM;
        mutex_unlock(&dat->lock);

        return mask;
}

int poll_test_release(struct inode *inode, struct file *file)
{
        pr_info("release is called by pid: %d\n", current->pid);
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = poll_test_open,
        .read = poll_test_read,
        .write = poll_test_write,
        .poll = poll_test_poll,
        .release = poll_test_release,
};

static int __init poll_test_init(void)
{
        int ret;
        drv_dat =
                (struct poll_test_drv_data *)kzalloc(sizeof(struct poll_test_drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }
        ret = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "test_chrdev_region");
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
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "poll_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                ret = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        drv_dat->kbuf[0] = '\0'; // always keep kbuf a valid string
        mutex_init(&drv_dat->lock);
        // 初始化等待队列
        init_waitqueue_head(&drv_dat->waitque);

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

static void __exit poll_test_exit(void)
{
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(poll_test_init);
module_exit(poll_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for poll_test");
