#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define KBUF_SIZE 32
struct container_data {
        dev_t dev_num;
        struct cdev cdev;
        struct device *dev;
        char kbuf[KBUF_SIZE];
};

static struct class *class;
static struct container_data *dat1;
static struct container_data *dat2;

int container_of_test_open(struct inode *inode, struct file *file)
{
        struct container_data *dat;

        // 通过container_of获取特定的container_data
        dat = container_of(inode->i_cdev, struct container_data, cdev);
        file->private_data = dat;

        pr_info("container_of_test_open is called, dev major: %d, dev minor: %d\n",
                MAJOR(dat->dev_num), MINOR(dat->dev_num));

        return 0;
}

ssize_t container_of_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct container_data *dat = file->private_data;
        size_t kbuf_len = strlen(dat->kbuf);
        size_t len = min(size, (size_t)(kbuf_len - *offset));

        if (*offset >= kbuf_len)
                return 0;

        if (copy_to_user(buf, dat->kbuf + *offset, len))
                return -EFAULT;

        *offset += len;

        return len;
}

ssize_t container_of_test_write(struct file *file, const char __user *buf, size_t size,
                                loff_t *offset)
{
        struct container_data *dat = file->private_data;
        size_t bytes = min(size, (size_t)(KBUF_SIZE - 1 - *offset));
        
        if (*offset >= KBUF_SIZE - 1)
                return -ENOSPC;        

        if (copy_from_user(dat->kbuf + *offset, buf, bytes) != 0)
                return -EFAULT;

        *offset += bytes;
        dat->kbuf[*offset] = '\0';
        
        return bytes;
}

int container_of_test_release(struct inode *inode, struct file *file)
{
        pr_info("container_of_test_release is called\n");
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = container_of_test_open,
        .read = container_of_test_read,
        .write = container_of_test_write,
        .release = container_of_test_release,
};

static int __init container_of_test_init(void)
{
        int err;
        dat1 = (struct container_data *)kmalloc(sizeof(struct container_data), GFP_KERNEL);
        if (dat1 == NULL) {
                pr_err("no memory dat1");
                err = -ENOMEM;
                goto kmalloc_dat1_fail;
        }
        memset(dat1, 0, sizeof(struct container_data));

        dat2 = (struct container_data *)kmalloc(sizeof(struct container_data), GFP_KERNEL);
        if (dat2 == NULL) {
                pr_err("no memory dat2");
                err = -ENOMEM;
                goto kmalloc_dat2_fail;
        }
        memset(dat2, 0, sizeof(struct container_data));

        err = alloc_chrdev_region(&dat1->dev_num, 0, 2, "container_of_test chrdev region");
        if (err < 0) {
                pr_err("alloc_chrdev_region error\n");
                goto alloc_chrdev_region_fail;
        }
        dat2->dev_num = MKDEV(MAJOR(dat1->dev_num), MINOR(dat1->dev_num) + 1);

        cdev_init(&dat1->cdev, &fops);
        err = cdev_add(&dat1->cdev, dat1->dev_num, 1);
        if (err < 0) {
                pr_err("cdev_add dat1 error\n");
                goto cdev_add_dat1_fail;
        }

        cdev_init(&dat2->cdev, &fops);
        err = cdev_add(&dat2->cdev, dat2->dev_num, 1);
        if (err < 0) {
                pr_err("cdev_add dat2 error\n");
                goto cdev_add_dat2_fail;
        }

        class = class_create(THIS_MODULE, "chrdev");
        if (IS_ERR(class)) {
                err = PTR_ERR(class);
                goto class_create_fail;
        }
        dat1->dev = device_create(class, NULL, dat1->dev_num, NULL, "container_of_test_dev%d", 0);
        if (IS_ERR(dat1->dev)) {
                err = PTR_ERR(dat1->dev);
                goto device_create_dat1_fail;
        }
        dat2->dev = device_create(class, NULL, dat2->dev_num, NULL, "container_of_test_dev%d", 1);
        if (IS_ERR(dat2->dev)) {
                err = PTR_ERR(dat2->dev);
                goto device_create_dat2_fail;
        }

        return 0;
device_create_dat2_fail:
        device_destroy(class, dat1->dev_num);
device_create_dat1_fail:
        class_destroy(class);
class_create_fail:
        cdev_del(&dat2->cdev);
cdev_add_dat2_fail:
        cdev_del(&dat1->cdev);
cdev_add_dat1_fail:
        unregister_chrdev_region(dat1->dev_num, 2);
alloc_chrdev_region_fail:
        kfree(dat2);
kmalloc_dat2_fail:
        kfree(dat1);
kmalloc_dat1_fail:
        return err;
}

static void __exit container_of_test_exit(void)
{
        device_destroy(class, dat1->dev_num);
        device_destroy(class, dat2->dev_num);
        class_destroy(class);
        cdev_del(&dat2->cdev);
        cdev_del(&dat1->cdev);
        unregister_chrdev_region(dat1->dev_num, 2);
        kfree(dat2);
        kfree(dat1);
        pr_info("container_of_test exit\n");
}
module_init(container_of_test_init);
module_exit(container_of_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for container_of");
