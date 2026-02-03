#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

// 自定义结构体
struct mykobj {
        struct kobject kobj; // 将kboject 嵌入到自定义的结构体中
};
        
static struct mykobj *mykobjp;

void mykobj_release(struct kobject *kobj)
{
        struct mykobj *p = container_of(kobj, struct mykobj, kobj);
        pr_info("mykobj (%p) free: %s\n", p, __func__);
        kfree(p);
}

// 定义atribute 对象 myattr1 和 myattr2
struct myattribute{
        struct kobj_attribute kobj_attr;
        int value;
};

// 自定义的show函数，用于读取属性值
ssize_t myattr1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
        ssize_t count;
        struct myattribute *myattr = container_of(attr, struct myattribute, kobj_attr);
        count = sprintf(buf, "%d\n", myattr->value);
        return count;
}

// 自定义的store函数，用于写入属性值
ssize_t myattr1_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                      size_t size)
{
        struct myattribute *myattr = container_of(attr, struct myattribute, kobj_attr);
        sscanf(buf, "%d\n", &myattr->value);
        return size;
}

// 自定义的show函数，用于读取属性值
ssize_t myattr2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
        ssize_t count;
        struct myattribute *myattr = container_of(attr, struct myattribute, kobj_attr);
        count = sprintf(buf, "%d\n", myattr->value);
        return count;
}

// 自定义的store函数，用于写入属性值
ssize_t myattr2_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                      size_t size)
{
        struct myattribute *myattr = container_of(attr, struct myattribute, kobj_attr);
        sscanf(buf, "%d\n", &myattr->value);
        return size;
}


static struct myattribute myattr1 = {
        .kobj_attr = __ATTR(myattr1, 0664, myattr1_show, myattr1_store),
        .value = 0,
};

static struct myattribute myattr2 = {
        .kobj_attr = __ATTR(myattr2, 0664, myattr2_show, myattr2_store),
        .value = 0,
};


struct attribute *mykobj_default_attrs[] = {
        &myattr1.kobj_attr.attr,
        &myattr2.kobj_attr.attr,
        NULL,
};

ssize_t myshow(struct kobject *kobj, struct attribute *attr, char *buf)
{
        struct kobj_attribute *kobj_attr = container_of(attr, struct kobj_attribute, attr);
        return kobj_attr->show(kobj, kobj_attr, buf);
}
ssize_t mystore(struct kobject *kobj, struct attribute *attr, const char *buf, size_t size)
{
        struct kobj_attribute *kobj_attr = container_of(attr, struct kobj_attribute, attr);
        return kobj_attr->store(kobj, kobj_attr, buf, size);
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
