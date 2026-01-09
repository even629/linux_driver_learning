#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>

static int major = 0;
module_param(major, int, S_IRUGO);
MODULE_DESCRIPTION("device num: major");

static int minor = 0;
module_param(minor, int, S_IRUGO);
MODULE_DESCRIPTION("device num: minor");

static char buf[] = "Hello World from kernel\n";

struct test_data {
        dev_t dev_num;
        struct cdev cdev;
        struct class *class;
        struct device *dev;
        char kbuf[32];
};

static struct test_data *dat;

static int file_private_test_open(struct inode *inode, struct file *file)
{
        pr_info("file_private_test_open is called\n");
        // 设置私有数据
        file->private_data = dat;
        pr_info("file->private_data is set\n");
        return 0;
}
static ssize_t file_private_test_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
        struct test_data *data;
        size_t len;

        if (*offset != 0) { // EOF
                return 0;
        }
        
        data = file->private_data;        
        len = min(size, strlen(dat->kbuf));

        pr_info("file_private_test_read is called\n");

        

        if (copy_to_user(buf, data->kbuf, len) != 0) {
                pr_err("copy_to_user error\n");
                return -EFAULT;
        }

        *offset += len;
        return sizeof(data->kbuf);
}

static int file_private_test_release(struct inode *inode, struct file *file)
{
        pr_info("file_private_test_release is called\n");
        return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .open = file_private_test_open,
        .read = file_private_test_read,
        .release = file_private_test_release,
};

static int __init file_private_test_init(void)
{
        int err;

        pr_info("file_private_test init\n");
        dat = (struct test_data *)kmalloc(sizeof(struct test_data), GFP_KERNEL);

        if (dat == NULL) {
                pr_err("no memory\n");
                goto kmalloc_fail;
        }
        strscpy(dat->kbuf, buf, sizeof(dat->kbuf));

        // 申请设备号
        if (major) {
                dat->dev_num = MKDEV(major, minor);
                err = register_chrdev_region(dat->dev_num, 1, "file_private_test chrdev region");
                if (err < 0) {
                        pr_err("register_chrdev_region error\n");
                        goto chrdev_region_fail;
                }
        } else {
                err = alloc_chrdev_region(&dat->dev_num, 0, 1, "file_private_test chrdev region");
                if (err < 0) {
                        pr_err("register_chrdev_region error\n");
                        goto chrdev_region_fail;
                }
        }
        // 添加cdev
        cdev_init(&dat->cdev, &fops);
        err = cdev_add(&dat->cdev, dat->dev_num, 1);
        if (err < 0) {
                pr_err("cdev_add error\n");
                goto cdev_add_fail;
        }
        // 创建设备节点
        dat->class = class_create(THIS_MODULE, "char_test");
        if (IS_ERR(dat->class)) {
                err = PTR_ERR(dat->class);
                pr_err("create class error");
                goto class_create_fail;
        }
        dat->dev = device_create(dat->class, NULL, dat->dev_num, NULL, "char_test_dev");
        if (IS_ERR(dat->dev)) {
                err = PTR_ERR(dat->dev);
                pr_err("create device error\n");
                goto device_create_fail;
        }

        return 0;
device_create_fail:
        class_destroy(dat->class);
class_create_fail:
        cdev_del(&dat->cdev);
cdev_add_fail:
        unregister_chrdev_region(dat->dev_num, 1);
chrdev_region_fail:
        kfree(dat);
kmalloc_fail:
        return err;
}

static void __exit file_private_test_exit(void)
{
        device_destroy(dat->class, dat->dev_num);
        class_destroy(dat->class);
        cdev_del(&dat->cdev);
        unregister_chrdev_region(dat->dev_num, 1);
        kfree(dat);
        pr_info("file_private_test exit\n");
}
module_init(file_private_test_init);
module_exit(file_private_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("This is a test sample for file private data");
