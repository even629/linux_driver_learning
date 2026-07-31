// SPDX-License-Identifier: GPL-2.0
/*
 * xarray_test.c —— 教学用内核模块：理解 Linux 内核 XArray
 *
 * XArray 是内核中用于稀疏索引数组的高效数据结构，核心特点：
 *   - 基于 radix tree，支持稀疏索引（0 到 2^32-1）
 *   - O(log n) 的查找/插入/删除
 *   - 支持 tagged pointer（XA_MARK_0/1/2）用于分类标记
 *   - 支持 store value（小整数直接存，无需指针）
 *   - 支持 xa_alloc 自动分配空闲 ID（常用于 inode 号等）
 *
 * 本模块演示 XArray 的完整 API 使用
 *
 * 使用方法：
 *   make build             # 交叉编译
 *   make deploy            # 部署到 buildroot rootfs
 *   make qemu              # 在 QEMU 中运行
 *   或 insmod xarray_test.ko && dmesg | tail -100
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/xarray.h>
#include <linux/slab.h>

/* ============================================================
 * 基础设施
 * ============================================================ */

/* 测试数据节点 */
struct xa_node_data {
	int key;
	int value;
};

/* 测试统计 */
static int g_passed;
static int g_failed;

/* 轻量断言宏 */
#define test_assert(cond, msg)  do {					\
	if (cond) {							\
		g_passed++;						\
		pr_info("  [PASS] %s\n", msg);				\
	} else {							\
		g_failed++;						\
		pr_err("  [FAIL] %s\n", msg);				\
	}								\
} while (0)

/* ============================================================
 * 第一层：基础操作（load/store/erase）
 * ============================================================ */

static void demo_basic_ops(void)
{
	struct xarray xa;
	struct xa_node_data *data;
	struct xa_node_data *old;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 1: Basic ops (load/store/erase)\n");
	pr_info("================================================\n");

	/* 初始化 XArray */
	xa_init(&xa);
	test_assert(xa_empty(&xa), "xa_init creates empty array");

	/* 创建测试数据 */
	data = kmalloc(sizeof(*data), GFP_KERNEL);
	data->key = 42;
	data->value = 420;

	/*
	 * xa_store: 在指定 index 存储 entry
	 * 返回旧 entry（首次存储返回 NULL）
	 */
	old = xa_store(&xa, 42, data, GFP_KERNEL);
	test_assert(old == NULL, "xa_store at empty index returns NULL");
	test_assert(!xa_empty(&xa), "array not empty after store");

	/*
	 * xa_load: 从指定 index 读取 entry
	 */
	old = xa_load(&xa, 42);
	test_assert(old == data, "xa_load returns stored entry");
	test_assert(old->value == 420, "stored value correct");

	/* 空槽返回 NULL */
	old = xa_load(&xa, 100);
	test_assert(old == NULL, "xa_load empty slot returns NULL");

	/*
	 * 稀疏索引演示：可以直接存储到大索引
	 */
	data = kmalloc(sizeof(*data), GFP_KERNEL);
	data->key = 1000;
	data->value = 10000;
	xa_store(&xa, 1000, data, GFP_KERNEL);

	old = xa_load(&xa, 1000);
	test_assert(old && old->value == 10000, "sparse index (1000) works");

	/*
	 * xa_erase: 删除指定 index 的 entry
	 * 返回被删除的 entry
	 */
	old = xa_erase(&xa, 42);
	test_assert(old && old->key == 42, "xa_erase returns erased entry");
	kfree(old);

	old = xa_load(&xa, 42);
	test_assert(old == NULL, "erased index returns NULL");

	/* 清理 */
	xa_erase(&xa, 1000);
	xa_destroy(&xa);
	test_assert(xa_empty(&xa), "xa_destroy leaves array empty");
}

/* ============================================================
 * 第二层：Value 存储（小整数直接存）
 * ============================================================ */

static void demo_store_value(void)
{
	struct xarray xa;
	void *entry;
	unsigned long val;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 2: Store value (inline integer)\n");
	pr_info("================================================\n");

	xa_init(&xa);

	/*
	 * xa_mk_value / xa_to_value / xa_is_value
	 * 小整数可以直接存储在 XArray 中，无需分配内存
	 * 原理：利用指针的最低位作为 tag（value entry 的 bit0 = 1）
	 * 支持范围：0 到 LONG_MAX（BITS_PER_XA_VALUE 位）
	 */
	entry = xa_mk_value(12345);
	xa_store(&xa, 0, entry, GFP_KERNEL);

	entry = xa_load(&xa, 0);
	test_assert(xa_is_value(entry), "loaded entry is a value");
	val = xa_to_value(entry);
	test_assert(val == 12345, "value round-trip correct");

	/* 存储 0 */
	entry = xa_mk_value(0);
	xa_store(&xa, 1, entry, GFP_KERNEL);

	entry = xa_load(&xa, 1);
	test_assert(xa_is_value(entry), "value 0 stored as value entry");
	test_assert(xa_to_value(entry) == 0, "value 0 round-trip correct");

	/* 大 value */
	entry = xa_mk_value(1000000);
	xa_store(&xa, 2, entry, GFP_KERNEL);
	entry = xa_load(&xa, 2);
	test_assert(xa_to_value(entry) == 1000000, "large value correct");

	xa_destroy(&xa);
}

/* ============================================================
 * 第三层：Tagged Pointer 与 Mark 机制
 * ============================================================ */

static void demo_mark_and_tag(void)
{
	struct xarray xa;
	struct xa_node_data *data, *d2, *d3, *found;
	unsigned long index;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 3: Marks and tagged pointers\n");
	pr_info("================================================\n");

	xa_init(&xa);

	/*
	 * XArray 提供 3 个 mark（XA_MARK_0, XA_MARK_1, XA_MARK_2）
	 * 用途：标记 entry 的某种状态，支持按 mark 遍历
	 * 典型应用：page cache 用 mark 表示 dirty 状态
	 */
	data = kmalloc(sizeof(*data), GFP_KERNEL);
	data->key = 1; data->value = 100;
	xa_store(&xa, 10, data, GFP_KERNEL);

	d2 = kmalloc(sizeof(*d2), GFP_KERNEL);
	d2->key = 2; d2->value = 200;
	xa_store(&xa, 20, d2, GFP_KERNEL);

	d3 = kmalloc(sizeof(*d3), GFP_KERNEL);
	d3->key = 3; d3->value = 300;
	xa_store(&xa, 30, d3, GFP_KERNEL);

	/* 设置 mark */
	xa_set_mark(&xa, 10, XA_MARK_0);
	xa_set_mark(&xa, 30, XA_MARK_0);

	/* 检查 mark */
	test_assert(xa_get_mark(&xa, 10, XA_MARK_0), "XA_MARK_0 set on index 10");
	test_assert(!xa_get_mark(&xa, 20, XA_MARK_0), "XA_MARK_0 not set on index 20");
	test_assert(xa_get_mark(&xa, 30, XA_MARK_0), "XA_MARK_0 set on index 30");

	/* Clear mark */
	xa_clear_mark(&xa, 10, XA_MARK_0);
	test_assert(!xa_get_mark(&xa, 10, XA_MARK_0), "XA_MARK_0 cleared from index 10");

	/*
	 * 按 mark 遍历：只迭代设置了指定 mark 的 entry
	 */
	pr_info("  Iterating entries with XA_MARK_0:\n");
	xa_for_each_marked(&xa, index, found, XA_MARK_0) {
		pr_info("    index=%lu, key=%d, value=%d\n",
			index, found->key, found->value);
	}
	/* index 30 是唯一设置了 MARK_0 的 */
	test_assert(xa_get_mark(&xa, 30, XA_MARK_0), "only index 30 has MARK_0");

	/*
	 * Tagged pointer：用低 2 位标记指针
	 * 与 mark 不同，tag 存储在 entry 而非节点中
	 */
	data->key = 99; data->value = 990;
	xa_store(&xa, 50, xa_tag_pointer(data, 1), GFP_KERNEL);

	found = xa_load(&xa, 50);
	test_assert(xa_pointer_tag(found) == 1, "tagged pointer tag=1");
	test_assert(xa_untag_pointer(found) == data, "untag recovers pointer");

	kfree(data);
	kfree(d2);
	kfree(d3);
	xa_destroy(&xa);
}

/* ============================================================
 * 第四层：迭代器（xa_for_each）
 * ============================================================ */

static void demo_iterator(void)
{
	struct xarray xa;
	struct xa_node_data *data;
	struct xa_node_data *found;
	unsigned long index;
	int count;
	int indices[] = { 0, 5, 100, 1000, 65535 };
	int i;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 4: Iterator (xa_for_each)\n");
	pr_info("================================================\n");

	xa_init(&xa);

	/*
	 * 插入稀疏分布的 entry
	 * index: 0, 5, 100, 1000, 65535
	 */

	for (i = 0; i < ARRAY_SIZE(indices); i++) {
		data = kmalloc(sizeof(*data), GFP_KERNEL);
		data->key = indices[i];
		data->value = indices[i] * 10;
		xa_store(&xa, indices[i], data, GFP_KERNEL);
	}

	/*
	 * xa_for_each: 遍历所有非 NULL entry
	 * 内部使用 xa_find / xa_find_after
	 */
	count = 0;
	xa_for_each(&xa, index, found) {
		pr_info("  index=%lu, value=%d\n", index, found->value);
		count++;
	}
	test_assert(count == 5, "xa_for_each iterates all 5 entries");

	/*
	 * xa_for_each_start: 从指定 index 开始遍历
	 */
	count = 0;
	xa_for_each_start(&xa, index, found, 50) {
		count++;
	}
	test_assert(count == 3, "xa_for_each_start from 50 finds 3 entries (100,1000,65535)");

	/* 清理 */
	xa_for_each(&xa, index, found) {
		xa_erase(&xa, index);
		kfree(found);
	}
	xa_destroy(&xa);
}

/* ============================================================
 * 第五层：xa_alloc 自动分配空闲 ID
 * ============================================================ */

static void demo_alloc(void)
{
	struct xarray xa;
	struct xa_node_data *data;
	u32 id;
	int ret;
	int i;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 5: xa_alloc (auto ID allocation)\n");
	pr_info("================================================\n");

	/*
	 * xa_init_flags + XA_FLAGS_ALLOC: 初始化支持自动分配的 XArray
	 * XA_FLAGS_TRACK_FREE 标记空闲 slot
	 * 典型应用：inode 号分配、文件描述符分配
	 */
	xa_init_flags(&xa, XA_FLAGS_ALLOC);

	/*
	 * xa_alloc: 自动找到空闲 index 并存储
	 * 返回 0 表示成功，id 返回分配的 index
	 */
	for (i = 0; i < 5; i++) {
		data = kmalloc(sizeof(*data), GFP_KERNEL);
		data->key = i;
		data->value = i * 100;

		ret = xa_alloc(&xa, &id, data, XA_LIMIT(0, 100), GFP_KERNEL);
		test_assert(ret == 0, "xa_alloc succeeds");
		pr_info("  allocated id=%u for key=%d\n", id, i);
	}

	/*
	 * 删除中间一个，再分配应该复用
	 */
	data = xa_erase(&xa, 2);
	kfree(data);

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	data->key = 99;
	data->value = 9900;
	ret = xa_alloc(&xa, &id, data, XA_LIMIT(0, 100), GFP_KERNEL);
	pr_info("  after erase id=2, re-allocated id=%u\n", id);
	test_assert(id == 2, "xa_alloc reuses freed id");

	/* 清理 */
	for (i = 0; i <= 4; i++) {
		data = xa_load(&xa, i);
		if (data)
			xa_erase(&xa, i);
		kfree(data);
	}
	xa_destroy(&xa);
}

/* ============================================================
 * 第六层：xa_cmpxchg 比较交换
 * ============================================================ */

static void demo_cmpxchg(void)
{
	struct xarray xa;
	struct xa_node_data *old, *new_data, *result;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 6: xa_cmpxchg (compare & swap)\n");
	pr_info("================================================\n");

	xa_init(&xa);

	/*
	 * xa_cmpxchg: 如果 index 处的值等于 old，则替换为 new
	 * 返回值等于 old 表示交换成功
	 */
	old = kmalloc(sizeof(*old), GFP_KERNEL);
	old->key = 1;
	old->value = 100;
	xa_store(&xa, 0, old, GFP_KERNEL);

	/* 正确的 old 值 → 交换成功 */
	new_data = kmalloc(sizeof(*new_data), GFP_KERNEL);
	new_data->key = 1;
	new_data->value = 200;
	result = xa_cmpxchg(&xa, 0, old, new_data, GFP_KERNEL);
	test_assert(result == old, "xa_cmpxchg succeeds with correct old");
	kfree(old);

	/* 验证新值 */
	result = xa_load(&xa, 0);
	test_assert(result->value == 200, "value updated to 200");

	/* 错误的 old 值 → 交换失败 */
	old = kmalloc(sizeof(*old), GFP_KERNEL);
	old->key = 1;
	old->value = 999;
	new_data = kmalloc(sizeof(*new_data), GFP_KERNEL);
	new_data->key = 1;
	new_data->value = 300;
	result = xa_cmpxchg(&xa, 0, old, new_data, GFP_KERNEL);
	test_assert(result != old, "xa_cmpxchg fails with wrong old value");
	kfree(old);
	kfree(new_data);

	/* 清理 */
	result = xa_erase(&xa, 0);
	kfree(result);
	xa_destroy(&xa);
}

/* ============================================================
 * 第七层：真实场景模拟（页缓存索引）
 * ============================================================ */

static void demo_page_cache_simulation(void)
{
	struct xarray xa;
	struct xa_node_data *page;
	unsigned long index;
	int i;
	int found_count;
	unsigned long pages[] = { 0, 3, 7, 15, 100 };
	int page_count = ARRAY_SIZE(pages);

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 7: Page cache index simulation\n");
	pr_info("================================================\n");

	/*
	 * 模拟内核 page cache 的核心机制：
	 * - 文件的 page cache 用 XArray 索引（index = page offset）
	 * - 支持 sparse access（文件可以有 hole）
	 * - 用 mark 标记 dirty page（需要回写）
	 */
	xa_init(&xa);

	/* 模拟写入文件的几个 page（稀疏分布） */

	pr_info("  Writing pages at offsets: ");
	for (i = 0; i < page_count; i++) {
		pr_cont("%lu ", pages[i]);

		page = kmalloc(sizeof(*page), GFP_KERNEL);
		page->key = pages[i];
		page->value = pages[i] * 4096; /* 模拟 page 物理地址 */
		xa_store(&xa, pages[i], page, GFP_KERNEL);

		/* 模拟 dirty page 标记 */
		if (i % 2 == 0) {
			xa_set_mark(&xa, pages[i], XA_MARK_0);
		}
	}
	pr_cont("\n");

	/* 读取 page */
	pr_info("  Reading page cache:\n");
	for (i = 0; i < page_count; i++) {
		page = xa_load(&xa, pages[i]);
		if (page) {
			pr_info("    page[%d] offset=%d, paddr=0x%x, dirty=%d\n",
				i, page->key, page->value,
				xa_get_mark(&xa, pages[i], XA_MARK_0));
		}
	}

	/* 遍历所有 dirty page（需要回写的） */
	pr_info("  Dirty pages (need writeback):\n");
	found_count = 0;
	xa_for_each_marked(&xa, index, page, XA_MARK_0) {
		pr_info("    offset=%lu, paddr=0x%x\n", index, page->value);
		found_count++;
	}
	test_assert(found_count == 3, "3 dirty pages (index 0,7,100)");

	/* 模拟 truncate：删除 offset >= 7 的 page */
	pr_info("  Truncating pages with offset >= 7\n");
	for (i = 0; i < page_count; i++) {
		if (pages[i] >= 7) {
			page = xa_erase(&xa, pages[i]);
			if (page) {
				pr_info("    dropped page offset=%d\n", page->key);
				kfree(page);
			}
		}
	}

	/* 验证剩余 page */
	pr_info("  Remaining pages:\n");
	xa_for_each(&xa, index, page) {
		pr_info("    offset=%lu\n", index);
	}

	/* 清理 */
	xa_for_each(&xa, index, page) {
		xa_erase(&xa, index);
		kfree(page);
	}
	xa_destroy(&xa);
}

/* ============================================================
 * 模块入口 / 出口
 * ============================================================ */

static int __init xarray_test_init(void)
{
	g_passed = 0;
	g_failed = 0;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("       xarray demo module loaded\n");
	pr_info("================================================\n");

	demo_basic_ops();
	demo_store_value();
	demo_mark_and_tag();
	demo_iterator();
	demo_alloc();
	demo_cmpxchg();
	demo_page_cache_simulation();

	/* 汇总 */
	pr_info("\n");
	pr_info("================================================\n");
	pr_info("              Test summary\n");
	pr_info("================================================\n");
	pr_info("   PASS: %-3d\n", g_passed);
	pr_info("   FAIL: %-3d\n", g_failed);
	pr_info("================================================\n");

	if (g_failed == 0)
		pr_info("All tests passed!\n");
	else
		pr_err("%d test(s) failed\n", g_failed);

	return 0;
}

static void __exit xarray_test_exit(void)
{
	pr_info("xarray_test module exit, bye!\n");
}

module_init(xarray_test_init);
module_exit(xarray_test_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("xarray demo module");
