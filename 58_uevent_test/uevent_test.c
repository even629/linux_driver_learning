#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>


struct kobject *mykobject01;
struct kset *mykset;
struct kobj_type mytype;


static int __init mykobj_uevent_init(void){
        int ret;

        // 创建并添加一个kset
        mykset = kset_create_and_add("mykset", NULL, NULL);

        // 初始化并添加kobject到kset
        mykobject01 = kzalloc(sizeof(*mykobject01), GFP_KERNEL);
        mykobject01->kset = mykset;
        ret = kobject_init_and_add(mykobject01, &mytype, NULL, "%s", "mykobject01");

        // 触发一个uevent 事件
        ret = kobject_uevent(mykobject01, KOBJ_CHANGE);
        
        return 0;
}


static void __exit mykobj_uevent_exit(void){
        kobject_put(mykobject01);
        kset_unregister(mykset);
}


module_init(mykobj_uevent_init);
module_exit(mykobj_uevent_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for kobject uevent");
