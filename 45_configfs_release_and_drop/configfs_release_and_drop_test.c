#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/configfs.h>

struct myitem {
        struct config_item conf_item;
};

void child_release(struct config_item *item)
{
        struct myitem *myitem = container_of(item, struct myitem, conf_item);
        kfree(myitem);
        pr_info("%s\n", __func__);
}

static struct configfs_item_operations child_item_ops = {
        .release = child_release,
};

static struct config_item_type child_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_item_ops = &child_item_ops,
};

struct config_item *root_make_item(struct config_group *group, const char *name)
{
        struct myitem *myitem;

        myitem = kzalloc(sizeof(*myitem), GFP_KERNEL);

        config_item_init_type_name(&myitem->conf_item, name, &child_config_item_type);

        pr_info("%s\n", __func__);
        return &myitem->conf_item;
}

// 当 configfs 中的配置组（group）被删除时(root目录下创建的group)，内核会调用该函数来处理与配置组相关的操作
void root_drop_item(struct config_group *group, struct config_item *item)
{
        struct myitem *myitem = container_of(item, struct myitem, conf_item);
        config_item_put(&myitem->conf_item);
        pr_info("%s\n", __func__);
}

static struct configfs_group_operations root_configfs_group_ops = {
        .make_item = root_make_item,
        .drop_item = root_drop_item,
};

static const struct config_item_type root_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_group_ops = &root_configfs_group_ops,
};

static struct configfs_subsystem test_configfs_subsystem = {
        .su_group = {
                .cg_item = {
                        .ci_namebuf = "myconfigfs",
                        .ci_type = &root_config_item_type,
                },
        },
};
static struct config_group child_group1;

static int __init configfs_test_init(void)
{
        config_group_init(&test_configfs_subsystem.su_group);
        configfs_register_subsystem(&test_configfs_subsystem);

        // 初始化配置组child_group1
        config_group_init_type_name(&child_group1, "child_group1", &child_config_item_type);
        configfs_register_group(&test_configfs_subsystem.su_group, &child_group1);
        return 0;
}

static void __exit configfs_test_exit(void)
{
        configfs_unregister_subsystem(&test_configfs_subsystem);
}

module_init(configfs_test_init);
module_exit(configfs_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test description for configfs");
