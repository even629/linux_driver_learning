#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

// 定义 kobject 指针
struct kobject *mykobject01;
struct kobject *mykobject02;

// 定义 kset 指针
struct kset *mykset;

// 定义kobj_type 结构体

struct kobj_type mytype;

static int __init kset_test_init(void)
{
        int ret;

        // 创建并添加mykset
        mykset = kset_create_and_add("mykset", NULL, NULL);

        // 创建并添加mykobject02
        mykobject01 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
        mykobject01->kset = mykset;
        ret = kobject_init_and_add(mykobject01, &mytype, NULL, "%s", "mykobject01");

        // 创建并添加mykobject01
        mykobject02 = kobject_create_and_add("mykobject02", mykobject01);

        return 0;
}

static void __exit kset_test_exit(void)
{
        // 释放 mykobject01 的引用计数
        kobject_put(mykobject01);
        // 释放 mykobject02 的引用计数
        kobject_put(mykobject02);

        kset_unregister(mykset);
}

module_init(kset_test_init);
module_exit(kset_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("This is a test sample for kset");
