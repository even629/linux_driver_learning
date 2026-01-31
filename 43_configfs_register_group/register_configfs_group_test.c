#include <linux/module.h>
#include <linux/init.h>
#include <linux/configfs.h>
// 在myconfigfs下创建mygroup
static struct config_group mygroup;

// mygroup_config_item_type，用于描述mygroup的配置项类型
static const struct config_item_type mygroup_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_item_ops = NULL,
        .ct_group_ops = NULL,
        .ct_attrs = NULL,
};

// myconfig_item_type，用于描述配置项类型的结构体
static const struct config_item_type myconfig_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_group_ops = NULL,
};

static struct configfs_subsystem myconfigfs_subsystem = {
        .su_group = {
                .cg_item = {
                        .ci_namebuf = "myconfigfs",
                        .ci_type = &myconfig_item_type,
                },
        },
};

static int __init myconfigfs_group_init(void)
{
        // 初始化配置组
        config_group_init(&myconfigfs_subsystem.su_group);
        // 注册子系统
        configfs_register_subsystem(&myconfigfs_subsystem);

        // 初始化配置组"mygroup"
        config_group_init_type_name(&mygroup, "mygroup", &mygroup_config_item_type);
        // 在子系统中配置组"mygroup"
        configfs_register_group(&myconfigfs_subsystem.su_group, &mygroup);

        return 0;
}

static void __exit myconfigfs_group_exit(void)
{
        // 注销子系统
        configfs_unregister_subsystem(&myconfigfs_subsystem);
}

module_init(myconfigfs_group_init);
module_exit(myconfigfs_group_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a sample for configfs: register group");
