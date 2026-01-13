#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/signal.h>

#define KBUF_SIZE 64

struct drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        wait_queue_head_t waitq;
        char kbuf[KBUF_SIZE];
        struct mutex lock;
        struct fasync_struct *fa;
};

static struct drv_data *drv_dat;

int signal_io_open(struct inode *inode, struct file *file)
{
        file->private_data = drv_dat;
        pr_info("signal_io_open is called by pid: %d\n", task_pid_nr(current));
        return 0;
}

ssize_t signal_io_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct drv_data *dat = file->private_data;
        size_t len;
        int ret;

        if (file->f_flags & O_NONBLOCK) { // 非阻塞方式
                mutex_lock(&drv_dat->lock);
                if (dat->kbuf[0] == '\0') {
                        mutex_unlock(&drv_dat->lock);
                        return -EAGAIN;
                }
                mutex_unlock(&drv_dat->lock);
        } else { // 阻塞方式
                ret = wait_event_interruptible(drv_dat->waitq, ({
                                   bool status;
                                   mutex_lock(&drv_dat->lock);
                                   status = (dat->kbuf[0] != '\0');
                                   mutex_unlock(&drv_dat->lock);
                                   status;
                }));
                if(ret < 0){
                        pr_info("signal_io_read called by pid: %d is interrupted\n", task_pid_nr(current));
                        return ret;
                }
        }

        mutex_lock(&dat->lock);
        len = min(size, strlen(dat->kbuf) + 1);        

        if (copy_to_user(buf, dat->kbuf, len) != 0) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }

        dat->kbuf[0] = '\0'; // clear kbuf

        mutex_unlock(&dat->lock);

        pr_info("signal_io_read is called by pid: %d\n", task_pid_nr(current));

        return len;
}
ssize_t signal_io_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        struct drv_data *dat = file->private_data;
        int len = min(size, (size_t)(KBUF_SIZE - 1));

        mutex_lock(&dat->lock);

        if (copy_from_user(drv_dat->kbuf, buf, len) != 0) {
                mutex_unlock(&dat->lock);
                return -EFAULT;
        }
        dat->kbuf[len] = '\0';
        mutex_unlock(&dat->lock);

        wake_up_interruptible(&dat->waitq);
        kill_fasync(&dat->fa, SIGIO, POLLIN);

        pr_info("signal_io_write is called by pid: %d\n", task_pid_nr(current));
        return len;
}
__poll_t signal_io_poll(struct file *file, struct poll_table_struct *p)
{
        struct drv_data *dat = file->private_data;
        __poll_t mask = 0;
        pr_info("signal_io_poll is called by pid: %d\n", task_pid_nr(current));

        poll_wait(file, &dat->waitq, p);

        mutex_lock(&dat->lock);

        if (dat->kbuf[0] != '\0') {
                mask |= POLLIN | POLLRDNORM;
        }

        mutex_unlock(&dat->lock);

        return mask;
}

int signal_io_fasync(int fd, struct file *file, int on)
{
        struct drv_data *dat = file->private_data;
        pr_info("signal_io_fasync is called by pid: %d\n", task_pid_nr(current));
        return fasync_helper(fd, file, on, &dat->fa);
}

int signal_io_release(struct inode *inode, struct file *file)
{
        pr_info("signal_io_release is called by pid: %d\n", task_pid_nr(current));

        /* 强制从异步队列移除 */
        signal_io_fasync(-1, file, 0);
        
        return 0;
}

struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = signal_io_open,
        .read = signal_io_read,
        .write = signal_io_write,
        .poll = signal_io_poll,
        .fasync = signal_io_fasync,
        .release = signal_io_release,
};

static int __init signal_io_init(void)
{
        int ret;

        drv_dat = (struct drv_data *)kzalloc(sizeof(struct drv_data), GFP_KERNEL);
        if (drv_dat == NULL) {
                ret = -ENOMEM;
                goto kzalloc_fail;
        }

        ret = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "chrdev_test_region");
        if (ret < 0)
                goto alloc_chrdev_region_fail;

        cdev_init(&drv_dat->cdev, &fops);
        drv_dat->cdev.owner = THIS_MODULE;
        ret = cdev_add(&drv_dat->cdev, drv_dat->dev_num, 1);
        if (ret < 0)
                goto cdev_add_fail;

        drv_dat->class = class_create(THIS_MODULE, "chrdev_test");
        if (IS_ERR(drv_dat->class)) {
                ret = PTR_ERR(drv_dat->class);
                goto class_create_fail;
        }
        drv_dat->dev =
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "signal_io_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                ret = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
        drv_dat->kbuf[0] = '\0';
        // 初始化等待队列
        init_waitqueue_head(&drv_dat->waitq);
        // 初始化互斥锁
        mutex_init(&drv_dat->lock);

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

static void __exit signal_io_exit(void)
{
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(signal_io_init);
module_exit(signal_io_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("test sample for signal io");
