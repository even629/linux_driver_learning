#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

struct kobject *mykobject1;
struct kobject *mykobject2;
struct kobject *mykobject3;

struct kobj_type mytype;

static int __init kobject_test_init(void)
{
        int ret = 0;

        // 创建 kobject
        // 方法一: kobject_create_and_add()
        mykobject1 = kobject_create_and_add("mykobject01", NULL);
        mykobject2 = kobject_create_and_add("mykobject02", mykobject1);

        // 方法二：kzalloc() + kobject_init_and_add()
        mykobject3 = kzalloc(sizeof(struct kobject), GFP_KERNEL);
        ret = kobject_init_and_add(mykobject3, &mytype, NULL, "%s", "mykobject03");

        return ret;
}
static void __exit kobject_test_exit(void)
{
        kobject_put(mykobject3);
        kobject_put(mykobject2);
        kobject_put(mykobject1);
}

module_init(kobject_test_init);
module_exit(kobject_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for kboject");
