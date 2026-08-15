#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/random.h>

struct my_node {
	struct rb_node rb;
	unsigned long data;
};

static struct rb_root my_tree = RB_ROOT;

#define my_rb_entry(ptr) rb_entry((ptr), struct my_node, rb)

/*
 * 查找节点 - O(logn)
 */
static struct my_node *my_rb_search(struct rb_root *root, unsigned long data)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct my_node *entry = my_rb_entry(node);

		if (data < entry->data)
			node = node->rb_left;
		else if (data > entry->data)
			node = node->rb_right;
		else
			return entry;
	}
	return NULL;
}

/*
 * 插入节点
 */
static bool my_rb_insert(struct rb_root *root, struct my_node *new_node)
{
	struct rb_node **link = &root->rb_node;
	struct rb_node *parent = NULL;
	unsigned long data = new_node->data;

	while (*link) {
		struct my_node *entry = my_rb_entry(*link);
		parent = *link;

		if (data < entry->data)
			link = &(*link)->rb_left;
		else if (data > entry->data)
			link = &(*link)->rb_right;
		else
			return false; /* key 已经存在，不需要重复插入 */
	}

	rb_link_node(&new_node->rb, parent, link);
	rb_insert_color(&new_node->rb, root);
	return true;
}

/*
 * 删除节点
 */
static void my_rb_erase(struct rb_root *root, struct my_node *node)
{
	rb_erase(&node->rb, root);
	kfree(node);
}

/*
 * 销毁整课树
 */
static void my_rb_destroy(struct rb_root *root)
{
	struct rb_node *node;
	while ((node = rb_first_postorder(root))) {
		rb_erase(node, root);
		kfree(my_rb_entry(node));
	}
	*root = RB_ROOT;
}

static int __init rbtree_test_init(void)
{
	int i;
	unsigned long keys[] = { 1, 2, 3, 4, 5, 6, 7 };
	int count = ARRAY_SIZE(keys);

	pr_info("%s is called\n", __func__);

	/* 插入测试 */
	for (i = 0; i < count; i++) {
		struct my_node *node = kzalloc(sizeof(*node), GFP_KERNEL);
		if (!node)
			return -ENOMEM;

		node->data = keys[i];

		if (my_rb_insert(&my_tree, node)) {
			pr_info("INSERT data=%lu OK\n", keys[i]);
		} else {
			pr_warn("INSERT data=%lu DUPLICATE\n", keys[i]);
			kfree(node);
		}
	}

	/* 查找测试 */
	{
		struct my_node *found = my_rb_search(&my_tree, 4);
		if (found)
			pr_info("SEARCH data=4 => %lu\n", found->data);
		else
			pr_warn("SEARCH data=4 => Not Found\n");

		found = my_rb_search(&my_tree, 99);
		if (found)
			pr_info("SEARCH data=99 => %lu\n", found->data);
		else
			pr_warn("SEARCH data=99 => Not Found\n");
	}
	/* 中序遍历 */
	{
		struct rb_node *node;

		pr_info("IN-ORDER TRAVERSAL:\n");

		for (node = rb_first(&my_tree); node; node = rb_next(node)) {
			struct my_node *entry = my_rb_entry(node);
			pr_info("    data=%lu\n", entry->data);
		}
	}
	/* 后序遍历 */
	{
		struct rb_node *node;

		pr_info("POST-ORDER TRAVERSAL:\n");

		for (node = rb_last(&my_tree); node; node = rb_prev(node)) {
			struct my_node *entry = my_rb_entry(node);
			pr_info("    data=%lu\n", entry->data);
		}
	}

	/* 获取最小/最大节点 */
	{
		struct my_node *min = my_rb_entry(rb_first(&my_tree));
		struct my_node *max = my_rb_entry(rb_last(&my_tree));
		pr_info("    MIN=%lu MAX=%lu\n", min->data, max->data);
	}

	/* 删除测试 */
	{
		struct my_node *to_del = my_rb_search(&my_tree, 4);
		if (to_del) {
			pr_info("ERASE data=4\n");
			my_rb_erase(&my_tree, to_del);
		}
	}

	/* 再次中序遍历确认结果 */
	{
		struct rb_node *node;

		pr_info("AFTER ERASE 4:\n");

		for (node = rb_first(&my_tree); node; node = rb_next(node)) {
			struct my_node *entry = my_rb_entry(node);
			pr_info("    data=%lu\n", entry->data);
		}
	}

	return 0;
}

static void __exit rbtree_test_exit(void)
{
	my_rb_destroy(&my_tree);
	pr_info("%s is called\n", __func__);
}

module_init(rbtree_test_init);
module_exit(rbtree_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("rbtree test");
