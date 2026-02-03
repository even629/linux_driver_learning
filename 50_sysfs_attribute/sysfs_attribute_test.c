#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

// 自定义结构体
struct mykobj {
        struct kobject kobj; // 将kboject 嵌入到自定义的结构体中
        int value1;
        int value2;
};

static struct mykobj *mykobjp;

void mykobj_release(struct kobject *kobj)
{
        struct mykobj *p = container_of(kobj, struct mykobj, kobj);
        pr_info("mykobj (%p) free: %s\n", p, __func__);
        kfree(p);
}

struct attribute myattr1 = {
        .name = "myattr1",
        .mode = 0666,
};
struct attribute myattr2 = {
        .name = "myattr2",
        .mode = 0666,
};

struct attribute *mykobj_default_attrs[] = {
        &myattr1,
        &myattr2,
        NULL,
};

ssize_t myshow(struct kobject *kobj, struct attribute *attr, char *buf)
{
        ssize_t count;
        struct mykobj *mykobj = container_of(kobj, struct mykobj, kobj);
        if (strcmp(attr->name, "myattr1") == 0) {
                count = sprintf(buf, "%d\n", mykobj->value1);
        } else if (strcmp(attr->name, "myattr2") == 0) {
                count = sprintf(buf, "%d\n", mykobj->value2);
        } else {
                count = 0;
        }

        return count;
}
ssize_t mystore(struct kobject *kobj, struct attribute *attr, const char *buf, size_t size)
{
        struct mykobj *mykobj = container_of(kobj, struct mykobj, kobj);
        if (strcmp(attr->name, "myattr1") == 0) {
                sscanf(buf, "%d\n", &mykobj->value1);
        } else if (strcmp(attr->name, "myattr2") == 0) {
                sscanf(buf, "%d\n", &mykobj->value2);
        }
        return size;
}

const struct sysfs_ops my_sysfs_ops = {
        .show = myshow,
        .store = mystore,
};

struct kobj_type mytype = {
        .release = mykobj_release,
        .default_attrs = mykobj_default_attrs,
        .sysfs_ops = &my_sysfs_ops,
};

static int __init sysfs_attribute_test_init(void)
{
        int ret;
        mykobjp = kzalloc(sizeof(struct mykobj), GFP_KERNEL);
        if (!mykobjp)
                return -ENOMEM;
        ret = kobject_init_and_add(&mykobjp->kobj, &mytype, NULL, "mykobject");

        return 0;
}

static void __exit sysfs_attribute_test_exit(void)
{
        kobject_put(&mykobjp->kobj);
}

module_init(sysfs_attribute_test_init);
module_exit(sysfs_attribute_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a sample for sysfs attribute");
