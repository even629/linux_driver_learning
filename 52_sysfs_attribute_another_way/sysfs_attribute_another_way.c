#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

static int value1;
static int value2;

// 自定义的show函数，用于读取属性值
ssize_t myattr1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n", value1);
}

// 自定义的store函数，用于写入属性值
ssize_t myattr1_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                      size_t size)
{
        sscanf(buf, "%d\n", &value1);
        return size;
}

// 自定义的show函数，用于读取属性值
ssize_t myattr2_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n", value2);
}

// 自定义的store函数，用于写入属性值
ssize_t myattr2_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf,
                      size_t size)
{
        sscanf(buf, "%d\n", &value2);
        return size;
}

static struct kobj_attribute kobj_attr1 = __ATTR(myattr1, 0664, myattr1_show, myattr1_store);
static struct kobj_attribute kobj_attr2 = __ATTR(myattr2, 0664, myattr2_show, myattr2_store);

static struct kobject *mykobj;

static int __init sysfs_attribute_test_init(void)
{
        int ret;

        mykobj = kobject_create_and_add("mykobject", NULL);
        ret = sysfs_create_file(mykobj, &kobj_attr1.attr);
        ret = sysfs_create_file(mykobj, &kobj_attr2.attr);

        return 0;
}

static void __exit sysfs_attribute_test_exit(void)
{
        kobject_put(mykobj);
}

module_init(sysfs_attribute_test_init);
module_exit(sysfs_attribute_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a sample for sysfs attribute");
