#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#define KBUF_CAPACITY 32
struct waitqueue_drv_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        char kbuf[KBUF_CAPACITY];
        bool kbuf_ready;
        wait_queue_head_t waitque;
};

static struct waitqueue_drv_data *drv_dat;



// DECLARE_WAIT_QUEUE_HEAD(waitque);

int waitqueue_test_open(struct inode *inode, struct file *file)
{
        file->private_data = drv_dat;
        
        pr_info("waitqueue_test_open is called\n");
        return 0;
}

ssize_t waitqueue_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct waitqueue_drv_data *dat = file->private_data;
        size_t len;
        int ret;

        // 如果以非阻塞方式打开
        if (file->f_flags & O_NONBLOCK) {
                if (!dat->kbuf_ready)
                        return -EAGAIN;
        } else { // 以阻塞方式打开
                ret = wait_event_interruptible(dat->waitque, dat->kbuf_ready == true);
                if (ret < 0) {
                        pr_err("waitque_test_read is interrupted\n");
                        return ret;
                }
        }

        len = min(size, (size_t)strlen(dat->kbuf));

        if (copy_to_user(buf, dat->kbuf + *offset, len) != 0)
                return -EFAULT;
        
        dat->kbuf_ready = false;

        return len;
}

ssize_t waitqueue_test_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
        struct waitqueue_drv_data *dat = file->private_data;
        size_t len = min(size, (size_t)(KBUF_CAPACITY - 1));

        if (copy_from_user(dat->kbuf, buf, len) != 0)
                return -EFAULT;

        dat->kbuf[len] = '\0';
        
        dat->kbuf_ready = true;
        wake_up_interruptible(&dat->waitque);

        return len;
}

int waitqueue_test_release(struct inode *inode, struct file *file)
{
        pr_info("waitqueue_test_release is called\n");        
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = waitqueue_test_open,
        .read = waitqueue_test_read,
        .write = waitqueue_test_write,
        .release = waitqueue_test_release,
};

static int __init waitqueue_test_init(void)
{
        int err;

        drv_dat =
                (struct waitqueue_drv_data *)kzalloc(sizeof(struct waitqueue_drv_data), GFP_KERNEL);
        if (drv_dat == NULL)
                goto kzalloc_fail;
        err = alloc_chrdev_region(&drv_dat->dev_num, 0, 1, "waitque_drv_chrdev_region");
        if (err < 0)
                goto alloc_chrdev_region_fail;

        cdev_init(&drv_dat->cdev, &fops);
        drv_dat->cdev.owner = THIS_MODULE;
        err = cdev_add(&drv_dat->cdev, drv_dat->dev_num, 1);
        if (err < 0)
                goto cdev_add_fail;

        drv_dat->class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(drv_dat->class)) {
                err = PTR_ERR(drv_dat->class);
                goto class_create_fail;
        }
        drv_dat->dev =
                device_create(drv_dat->class, NULL, drv_dat->dev_num, NULL, "waitqueue_test%d", 0);
        if (IS_ERR(drv_dat->dev)) {
                err = PTR_ERR(drv_dat->dev);
                goto device_create_fail;
        }
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
        return err;
}

static void __exit waitqueue_test_exit(void)
{
        device_destroy(drv_dat->class, drv_dat->dev_num);
        class_destroy(drv_dat->class);
        cdev_del(&drv_dat->cdev);
        unregister_chrdev_region(drv_dat->dev_num, 1);
        kfree(drv_dat);
}

module_init(waitqueue_test_init);
module_exit(waitqueue_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("waitqueue sample");
