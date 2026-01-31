#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/configfs.h>

struct myitem {
        struct config_item conf_item;
        int size;
        void *addr;
};
struct mygroup {
        struct config_group conf_group;
};

void child_item_release(struct config_item *item)
{
        struct myitem *myitem = container_of(item, struct myitem, conf_item);
        kfree(myitem);
        pr_info("%s\n", __func__);
}

static struct configfs_item_operations child_item_ops = {
        .release = child_item_release,
};

// static struct configfs_group_operations child_group_ops = {

// };

ssize_t myread_show(struct config_item *item, char *page)
{
        struct myitem *myitem = container_of(item, struct myitem, conf_item);
        if (myitem->size > 0 && myitem->addr != NULL)
                memcpy(page, myitem->addr, myitem->size);
        pr_info("%s\n", __func__);
        return myitem->size;
}
ssize_t mywrite_store(struct config_item *item, const char *page, size_t size)
{
        struct myitem *myitem = container_of(item, struct myitem, conf_item);        
        myitem->addr = kmemdup(page, size, GFP_KERNEL);
        myitem->size = size;
        pr_info("%s\n", __func__);
        return myitem->size;
}

CONFIGFS_ATTR_RO(my, read);
CONFIGFS_ATTR_WO(my, write);

struct configfs_attribute *child_item_attribute[] = {
        &myattr_read,
        &myattr_write,
        NULL,
};

static struct config_item_type child_item_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_item_ops = &child_item_ops,
        .ct_attrs = child_item_attribute,
};

// 二级文件夹
static struct config_item_type child_group_config_item_type = {
        .ct_owner = THIS_MODULE,
        // .ct_group_ops = &child_group_ops,
        .ct_group_ops = NULL,
};

// root folder

struct config_item *root_make_item(struct config_group *group, const char *name)
{
        struct myitem *myitem;

        myitem = kzalloc(sizeof(*myitem), GFP_KERNEL);
        config_item_init_type_name(&myitem->conf_item, name, &child_item_config_item_type);
        pr_info("%s\n", __func__);
        return &myitem->conf_item;
}

struct config_group *root_make_group(struct config_group *group, const char *name)
{
        struct mygroup *mygroup;

        mygroup = kzalloc(sizeof(*mygroup), GFP_KERNEL);
        config_group_init_type_name(&mygroup->conf_group, name, &child_group_config_item_type);
        pr_info("%s\n", __func__);
        return &mygroup->conf_group;
}

void root_drop_item(struct config_group *group, struct config_item *item)
{
        config_item_put(item);
        pr_info("%s\n", __func__);
}

static struct configfs_group_operations root_group_ops = {
        .make_item = root_make_item,
        .make_group = root_make_group,
        .drop_item = root_drop_item,
};

static struct config_item_type root_config_item_type = {
        .ct_owner = THIS_MODULE,
        .ct_group_ops = &root_group_ops,
};

static struct configfs_subsystem configfs_test_subsystem = {
        .su_group = {
                .cg_item = {
                        .ci_namebuf = "myconfigfs",
                        .ci_type = &root_config_item_type,
                },
        },
};

static struct config_group mygroup;

static int __init configfs_test_init(void)
{
        config_group_init(&configfs_test_subsystem.su_group);
        configfs_register_subsystem(&configfs_test_subsystem);

        config_group_init_type_name(&mygroup, "mygroup", &child_group_config_item_type);
        configfs_register_group(&configfs_test_subsystem.su_group, &mygroup);

        return 0;
}

static void __exit configfs_test_exit(void)
{
        configfs_unregister_group(&mygroup);
}

module_init(configfs_test_init);
module_exit(configfs_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for configfs");
