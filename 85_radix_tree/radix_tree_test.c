#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>

struct my_data {
	unsigned long id;
	char name[32];
};

/* 全局 radix tree 以及保护锁 */
static RADIX_TREE(my_rtree, GFP_ATOMIC);
static DEFINE_SPINLOCK(my_rtree_lock);

/* 辅助函数 */
static struct my_data *create_data(unsigned long id, const char *name)
{
	struct my_data *d = kmalloc(sizeof(*d), GFP_KERNEL);
	if (d) {
		d->id = id;
		strscpy(d->name, name, sizeof(d->name));
	}
	return d;
}

/* 核心操作函数封装 */
static int safe_insert(unsigned long index, struct my_data *data)
{
	int ret;

	/* 预分配节点内存(允许睡眠) */
	ret = radix_tree_preload(GFP_KERNEL);
	if (ret)
		return ret;

	spin_lock(&my_rtree_lock);
	ret = radix_tree_insert(&my_rtree, index, data);
	spin_unlock(&my_rtree_lock);

	radix_tree_preload_end();

	return ret;
}

/* 查找节点 RCU 读侧安全 */
static struct my_data *safe_lookup(unsigned long index)
{
	struct my_data *data;

	rcu_read_lock();
	data = radix_tree_lookup(&my_rtree, index);
	rcu_read_unlock();

	return data;
}

/* 删除并释放节点 */
static void safe_delete(unsigned long index)
{
	struct my_data *data;

	spin_lock(&my_rtree_lock);
	data = radix_tree_delete(&my_rtree, index);
	spin_unlock(&my_rtree_lock);

	kfree(data);
}

/* 使用tag批量标记和检索 */
static void demo_tag_operations(void)
{
	struct my_data *d;
	void **slot;
	struct radix_tree_iter iter;

	pr_info("TAG Operations\n");

	/* 给 index=100 打上tag 0 */
	spin_lock(&my_rtree_lock);
	radix_tree_tag_set(&my_rtree, 100, 0);
	spin_unlock(&my_rtree_lock);

	rcu_read_lock();
	radix_tree_for_each_tagged (slot, &my_rtree, &iter, 0, 0) {
                d = radix_tree_deref_slot(slot);
                if (unlikely(radix_tree_deref_retry(d))){
                        slot = radix_tree_iter_retry(&iter);
                        continue;
                }
                if (d)
                        pr_info("TAGGED idex=%lu name=%s\n", iter.index, d->name);
	}
}

/* ========== 模块入口 ========== */
static int __init radix_tree_demo_init(void)
{
    struct my_data *d;
    int ret;

    pr_info("rtree_demo: === Module Loaded ===\n");

    /* RADIX_TREE() 宏已静态初始化，无需手动 INIT_RADIX_TREE */

    /* --- 插入测试 --- */
    d = create_data(42, "hello");
    ret = safe_insert(42, d);
    pr_info("rtree_demo: INSERT index=42 ret=%d\n", ret);

    d = create_data(100, "world");
    ret = safe_insert(100, d);
    pr_info("rtree_demo: INSERT index=100 ret=%d\n", ret);

    /* 测试重复插入 */
    d = create_data(42, "duplicate");
    ret = safe_insert(42, d);
    pr_info("rtree_demo: INSERT DUP index=42 ret=%d (expect -EEXIST)\n", ret);
    kfree(d); /* 重复插入失败，手动释放 */

    /* --- 查找测试 --- */
    d = safe_lookup(42);
    pr_info("rtree_demo: LOOKUP 42 => %s\n", d ? d->name : "NULL");

    d = safe_lookup(999);
    pr_info("rtree_demo: LOOKUP 999 => %s\n", d ? d->name : "NULL");

    /* --- Tag 测试 --- */
    demo_tag_operations();

    /* --- 删除测试 --- */
    safe_delete(42);
    d = safe_lookup(42);
    pr_info("rtree_demo: AFTER DELETE 42 => %s\n", d ? d->name : "NULL");

    return 0;
}

/* ========== 模块卸载：安全遍历并释放所有剩余节点 ========== */
static void __exit radix_tree_demo_exit(void)
{
    struct my_data *d;
    void **slot;
    struct radix_tree_iter iter; /* 正确的迭代器类型 */

    rcu_read_lock();
    radix_tree_for_each_slot(slot, &my_rtree, &iter, 0) {
        d = radix_tree_deref_slot(slot);
        if (unlikely(radix_tree_deref_retry(d))) {
            slot = radix_tree_iter_retry(&iter); /* retry 传入 iter */
            continue;
        }
        if (d) {
            /* 必须在锁内删除，且使用 iter.index */
            spin_lock(&my_rtree_lock);
            radix_tree_delete(&my_rtree, iter.index);
            spin_unlock(&my_rtree_lock);
            kfree(d);
        }
    }
    rcu_read_unlock();

    pr_info("rtree_demo: === Module Unloaded ===\n");
}

module_init(radix_tree_demo_init);
module_exit(radix_tree_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("test for radix_tree_test");
