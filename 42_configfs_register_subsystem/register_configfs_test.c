#include <linux/module.h>
#include <linux/init.h>
#include <linux/configfs.h>


static const struct config_item_type myconfig_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_item_ops = NULL,
        .ct_group_ops = NULL,
        .ct_attrs = NULL,
        // .ct_bin_attrs = NULL,
};

static struct configfs_subsystem myconfigfs_subsystem = {
        .su_group = {
                .cg_item = {
                        .ci_namebuf = "myconfigfs",
                        .ci_type = &myconfig_item_type,
                },
        },
};

static int __init myconfigfs_init(void)
{
        // 初始化config_group
        config_group_init(&myconfigfs_subsystem.su_group);
        // 注册子系统
        configfs_register_subsystem(&myconfigfs_subsystem);

        return 0;
}

static void __exit myconfigfs_exit(void)
{
        configfs_unregister_subsystem(&myconfigfs_subsystem);
}

module_init(myconfigfs_init);
module_exit(myconfigfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for configfs ");
