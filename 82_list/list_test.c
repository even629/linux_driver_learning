#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

#define CNT 10

struct my_data {
	int val;
	struct list_head list;
};

LIST_HEAD(my_list);

static int __init list_test_init(void)
{
	int i, ret = 0;
	struct my_data *entry, *tmp;

	for (i = 0; i < CNT; i++) {
		entry = kzalloc(sizeof(*entry), GFP_KERNEL);
		if (!entry) {
			ret = -ENOMEM;
			goto cleanup;
		}
		entry->val = i * 2;
		INIT_LIST_HEAD(&entry->list);
		list_add_tail(&entry->list, &my_list);
	}

	list_for_each_entry (entry, &my_list, list) {
		pr_info("val is %d\n", entry->val);
	}

	return 0;
cleanup:
	list_for_each_entry_safe (entry, tmp, &my_list, list) {
		list_del(&entry->list);
		kfree(entry);
	}

	return ret;
}

static void __exit list_test_exit(void)
{
	struct my_data *entry, *tmp;
	list_for_each_entry_safe (entry, tmp, &my_list, list) {
		pr_info("del %d\n", entry->val);
		list_del(&entry->list);
		kfree(entry);
	}

	pr_info("%s is called\n", __func__);
}

module_init(list_test_init);
module_exit(list_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("list test");
