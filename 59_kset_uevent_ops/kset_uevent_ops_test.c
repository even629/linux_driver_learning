#include <linux/module.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/slab.h>

struct kobject *mykobject01;
struct kobject *mykobject02;
struct kset *mykset;
struct kobj_type myktype;

int myfilter(struct kset *kset, struct kobject *kobj)
{
        if (strcmp(kobj->name, "mykobject01") == 0) {
                return 0; // 返回0表示过滤掉，不发送uevent
        }
        return 1;
}
const char *myname(struct kset *kset, struct kobject *kobj)
{
        return "my_kset";
}
int myuevent(struct kset *kset, struct kobject *kobj, struct kobj_uevent_env *env)
{
        add_uevent_var(env, "MYDEVICE=%s", "RK3568");
        return 0;
}

static struct kset_uevent_ops my_uevent_ops = {
        .filter = myfilter,
        .name = myname,
        .uevent = myuevent,
};

static int __init kset_uevent_ops_init(void)
{
        int ret = 0;
        // 创建并添加一个kset
        mykset = kset_create_and_add("mykset", &my_uevent_ops, NULL);

        // 创建kboject01
        mykobject01 = kzalloc(sizeof(*mykobject01), GFP_KERNEL);
        mykobject01->kset = mykset;
        ret = kobject_init_and_add(mykobject01, &myktype, NULL, "mykobject01");

        // 创建kboject02
        mykobject02 = kzalloc(sizeof(*mykobject02), GFP_KERNEL);
        mykobject02->kset = mykset;
        ret = kobject_init_and_add(mykobject02, &myktype, NULL, "mykobject02");

        // 触发uevent事件
        ret = kobject_uevent(mykobject01, KOBJ_ADD);
        ret = kobject_uevent(mykobject02, KOBJ_ADD);

        return ret;
}

static void __exit kset_uevent_ops_exit(void)
{
        kobject_put(mykobject01);
        kobject_put(mykobject02);
        kset_unregister(mykset);
}

module_init(kset_uevent_ops_init);
module_exit(kset_uevent_ops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for kset_uevent_ops");
