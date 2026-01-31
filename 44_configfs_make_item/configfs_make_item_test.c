#include <linux/init.h>
#include <linux/module.h>
#include <linux/configfs.h>
#include <linux/slab.h>

static struct config_group mygroup;

struct myitem {
        struct config_item item;
};

void myitem_release(struct config_item *item)
{
        struct myitem *mitem = container_of(item, struct myitem, item);
        kfree(mitem);
        pr_info("%s\n", __func__);
}
struct configfs_item_operations myitem_ops = {
        .release = myitem_release,
};

static struct config_item_type mygroup_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_item_ops = &myitem_ops,
};

struct config_item *rootgroup_make_item(struct config_group *group, const char *name)
{
        struct myitem *my_config_item;
        pr_info("%s\n", __func__);
        my_config_item = kzalloc(sizeof(*my_config_item), GFP_KERNEL);
        config_item_init_type_name(&my_config_item->item, name, &mygroup_config_item_type);
        return &my_config_item->item;
}

struct configfs_group_operations rootgroup_ops = {
        .make_item = rootgroup_make_item,
};

static struct config_item_type rootgroup_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_group_ops = &rootgroup_ops,
};

static struct configfs_subsystem myconfigfs_subsystem = {
        .su_group = {
                .cg_item = {
                        .ci_namebuf = "myconfigfs",
                        .ci_type = &rootgroup_config_item_type,
                },
        },
};

static int __init myconfigfs_test_init(void)
{
        // 初始化配置组
        config_group_init(&myconfigfs_subsystem.su_group);
        // 注册子系统
        configfs_register_subsystem(&myconfigfs_subsystem);

        // 初始化配置组mygroup
        config_group_init_type_name(&mygroup, "mygroup", &mygroup_config_item_type);
        // 将mygroup挂到myconfigfs_subsystem.su_group下面
        configfs_register_group(&myconfigfs_subsystem.su_group, &mygroup);

        return 0;
}

static void __exit myconfigfs_test_exit(void)
{
        // 注销myconfigfs_subsystem
        configfs_unregister_subsystem(&myconfigfs_subsystem);
}

module_init(myconfigfs_test_init);
module_exit(myconfigfs_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("this is a test sample for configfs");
