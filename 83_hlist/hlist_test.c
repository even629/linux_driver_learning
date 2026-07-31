// SPDX-License-Identifier: GPL-2.0
/*
 * hlist_test.c —— 教学用内核模块：理解 Linux 内核 hlist（哈希链表）
 *
 * hlist 是内核中广泛用于哈希表的数据结构，核心特点：
 *   - 头节点只有单个 first 指针（比双向链表省一个指针）
 *   - 节点通过 pprev 指向前驱的 next 字段，实现 O(1) 删除
 *   - 代价是无法 O(1) 访问尾节点
 *
 * 本模块聚焦 hlist 最典型的应用场景：哈希表
 *
 * 使用方法：
 *   make build             # 交叉编译
 *   make deploy            # 部署到 buildroot rootfs
 *   make qemu              # 在 QEMU 中运行
 *   或 insmod hlist_test.ko && dmesg | tail -100
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("hlist hashtable demo module");

/* ============================================================
 * 基础设施
 * ============================================================ */

/* 哈希节点：模拟实际使用中嵌入 hlist_node 的数据结构 */
struct hlist_item {
	int key;			/* 键，用于标识节点 */
	int value;			/* 值，模拟实际负载数据 */
	struct hlist_node node;		/* 嵌入的 hlist 节点 */
};

/* 辅助宏：通过 hlist_node 指针获取父结构体指针 */
#define item_entry(ptr)  hlist_entry(ptr, struct hlist_item, node)

/* ============================================================
 * 哈希表演示
 * ============================================================ */

#define HASH_TABLE_SIZE  8

static void demo_hash_table(void)
{
	struct hlist_head hashtable[HASH_TABLE_SIZE];
	struct hlist_item *items;
	/* 精心选择的 key，确保产生哈希冲突 */
	int keys[] = { 5, 13, 21, 8, 16, 24, 7, 15, 23, 6, 14, 22 };
	int n = ARRAY_SIZE(keys);
	int i;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("          hlist hashtable demo\n");
	pr_info("================================================\n");

	/* 初始化所有哈希桶 */
	for (i = 0; i < HASH_TABLE_SIZE; i++)
		INIT_HLIST_HEAD(&hashtable[i]);

	/* 动态分配所有节点 */
	items = kcalloc(n, sizeof(*items), GFP_KERNEL);
	if (!items) {
		pr_err("memory alloc failed\n");
		return;
	}

	/*
	 * 插入阶段：将 key 取模后放入对应桶
	 * 哈希冲突时用头插法形成链
	 * 内核的 rhashtable 也是类似机制
	 */
	pr_info("\n--- Insert %d keys into %d buckets (key %%%d) ---\n",
		n, HASH_TABLE_SIZE, HASH_TABLE_SIZE);

	for (i = 0; i < n; i++) {
		int hash = keys[i] % HASH_TABLE_SIZE;

		items[i].key = keys[i];
		items[i].value = keys[i] * 10;  /* 模拟负载 */
		hlist_add_head(&items[i].node, &hashtable[hash]);
		pr_info("  key=%2d -> bucket[%d]\n", keys[i], hash);
	}

	/* 打印每个桶的内容 */
	pr_info("\n--------------- Bucket layout ---------------\n");
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct hlist_node *pos;
		int count = 0;

		pr_info("  bucket[%d]: ", i);
		hlist_for_each(pos, &hashtable[i]) {
			if (count > 0)
				pr_cont(" -> ");
			pr_cont("%d", item_entry(pos)->key);
			count++;
		}
		pr_cont("%s  (chain len: %d)\n", count ? "" : "(empty)", count);
	}

	/*
	 * 查找阶段：在哈希表中搜索指定 key
	 * 使用 hlist_for_each_entry 宏直接获取父结构体
	 */
	pr_info("\n--- Lookup demo ---\n");
	{
		int search_keys[] = { 13, 24, 100 };
		int j;

		for (j = 0; j < ARRAY_SIZE(search_keys); j++) {
			int key = search_keys[j];
			int hash = key % HASH_TABLE_SIZE;
			struct hlist_item *found = NULL;

			hlist_for_each_entry(found, &hashtable[hash], node) {
				if (found->key == key)
					break;
			}
			if (found && found->key == key)
				pr_info("  lookup key=%d: found! value=%d (bucket[%d])\n",
					key, found->value, hash);
			else
				pr_info("  lookup key=%d: not found\n", key);
		}
	}

	/*
	 * 删除阶段：从哈希表中删除指定 key
	 * hlist_del 是 O(1) 操作，这就是 pprev 的威力
	 */
	pr_info("\n--- Delete demo ---\n");
	{
		int del_keys[] = { 13, 24 };
		int j;

		for (j = 0; j < ARRAY_SIZE(del_keys); j++) {
			int key = del_keys[j];
			int hash = key % HASH_TABLE_SIZE;
			struct hlist_item *target = NULL;

			hlist_for_each_entry(target, &hashtable[hash], node) {
				if (target->key == key)
					break;
			}
			if (target && target->key == key) {
				hlist_del_init(&target->node);
				pr_info("  delete key=%d success (bucket[%d])\n", key, hash);
			} else {
				pr_info("  delete key=%d failed, not found\n", key);
			}
		}
	}

	/* 删除后打印桶分布 */
	pr_info("\n--- Bucket layout after delete ---\n");
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		struct hlist_node *pos;
		int count = 0;

		pr_info("  bucket[%d]: ", i);
		hlist_for_each(pos, &hashtable[i]) {
			if (count > 0)
				pr_cont(" -> ");
			pr_cont("%d", item_entry(pos)->key);
			count++;
		}
		pr_cont("%s  (chain len: %d)\n", count ? "" : "(empty)", count);
	}

	/*
	 * 演示 hlist_move_list 在 rehash 中的应用
	 * 当哈希表扩容/缩容时，需要将旧桶的链表整体搬移到新桶
	 * 这是一个 O(1) 操作！
	 */
	pr_info("\n--- hlist_move_list demo (rehash) ---\n");
	pr_info("  move bucket[5] to new_bucket\n");

	{
		struct hlist_head new_bucket;
		struct hlist_node *pos;

		INIT_HLIST_HEAD(&new_bucket);
		hlist_move_list(&hashtable[5], &new_bucket);

		pr_info("  new_bucket: ");
		hlist_for_each(pos, &new_bucket) {
			pr_cont("%d ", item_entry(pos)->key);
		}
		pr_cont("\n");
		pr_info("  old bucket[5]: %s\n",
			hlist_empty(&hashtable[5]) ? "empty" : "non-empty");
	}

	kfree(items);
}

/* ============================================================
 * 模块入口 / 出口
 * ============================================================ */

static int __init hlist_test_init(void)
{
	pr_info("\n");
	pr_info("================================================\n");
	pr_info("       hlist hashtable demo module loaded\n");
	pr_info("================================================\n");

	demo_hash_table();

	pr_info("\ndemo done\n");
	return 0;
}

static void __exit hlist_test_exit(void)
{
	pr_info("hlist_test module exit, bye!\n");
}

module_init(hlist_test_init);
module_exit(hlist_test_exit);
