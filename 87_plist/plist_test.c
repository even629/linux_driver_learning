// SPDX-License-Identifier: GPL-2.0
/*
 * plist_test.c —— 教学用内核模块：理解 Linux 内核 plist（优先级链表）
 *
 * plist 是一个按优先级排序的双向链表，核心特点：
 *   - 节点按优先级从高到低排序（INT_MIN 最高，INT_MAX 最低）
 *   - 两层结构：prio_list（不同优先级）+ node_list（所有节点）
 *   - O(K) 插入，O(1) 删除，O(K) 优先级修改（K = RT 优先级级数）
 *   - 同优先级节点按 FIFO 排序
 *
 * 典型应用：RT scheduler 的 pushable_tasks
 *
 * 注意：内核的 plist_add/del/requeue 未 export 给模块，
 * 这里用 my_plist_ 前缀实现简化版来演示核心原理。
 *
 * 使用方法：
 *   make build             # 交叉编译
 *   make deploy            # 部署到 buildroot rootfs
 *   make qemu              # 在 QEMU 中运行
 *   或 insmod plist_test.ko && dmesg | tail -100
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/plist.h>
#include <linux/slab.h>

/* ============================================================
 * 简化版 plist 核心函数（内核未 export，自行实现）
 * 使用 my_plist_ 前缀避免与 plist.h 冲突
 * ============================================================ */

/*
 * my_plist_add: 按优先级插入节点
 * 算法：遍历 node_list 找到第一个 prio 大于等于新节点的位置，插入其前面
 * 同优先级时 LIFO（新节点插在同级前面，这是内核 plist 的行为）
 */
static void my_plist_add(struct plist_node *node, struct plist_head *head)
{
	struct list_head *pos;
	struct plist_node *iter;

	/*
	 * 查找插入位置：找到第一个 prio 大于等于新节点的位置
	 * 使用 >= 保证同优先级时新节点插在同级前面（LIFO）
	 */
	list_for_each(pos, &head->node_list) {
		iter = list_entry(pos, struct plist_node, node_list);
		if (iter->prio >= node->prio) {
			list_add_tail(&node->node_list, pos);
			return;
		}
	}
	/* 优先级最低，插到最后 */
	list_add_tail(&node->node_list, &head->node_list);
}

/*
 * my_plist_del: O(1) 删除节点
 */
static void my_plist_del(struct plist_node *node, struct plist_head *head)
{
	list_del_init(&node->node_list);
}

/*
 * my_plist_requeue: 修改优先级后重新排序
 * 先删除再重新插入
 */
static void my_plist_requeue(struct plist_node *node, struct plist_head *head)
{
	if (!plist_node_empty(node))
		my_plist_del(node, head);
	my_plist_add(node, head);
}

/* ============================================================
 * 基础设施
 * ============================================================ */

/* 测试数据节点 */
struct plist_data {
	int id;
	int value;
	struct plist_node node;
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
 * 第一层：基础操作（add/del/iterate）
 * ============================================================ */

static void demo_basic_ops(void)
{
	PLIST_HEAD(head);
	struct plist_data *d1, *d2, *d3, *pos;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 1: Basic ops (add/del/iterate)\n");
	pr_info("================================================\n");

	test_assert(plist_head_empty(&head), "plist starts empty");

	/*
	 * 创建三个不同优先级的节点
	 * 优先级：d1(10) > d2(20) > d3(30)
	 * 数值越小优先级越高
	 */
	d1 = kmalloc(sizeof(*d1), GFP_KERNEL);
	d1->id = 1; d1->value = 100;
	plist_node_init(&d1->node, 10);

	d2 = kmalloc(sizeof(*d2), GFP_KERNEL);
	d2->id = 2; d2->value = 200;
	plist_node_init(&d2->node, 20);

	d3 = kmalloc(sizeof(*d3), GFP_KERNEL);
	d3->id = 3; d3->value = 300;
	plist_node_init(&d3->node, 30);

	/*
	 * plist_add: 按优先级插入
	 * 插入顺序不影响最终排序
	 */
	my_plist_add(&d2->node, &head);
	my_plist_add(&d1->node, &head);
	my_plist_add(&d3->node, &head);

	test_assert(!plist_head_empty(&head), "list not empty after adds");

	/*
	 * plist_first: 返回最高优先级的节点
	 */
	test_assert(plist_first(&head) == &d1->node, "first node has highest prio (10)");
	test_assert(plist_last(&head) == &d3->node, "last node has lowest prio (30)");

	/*
	 * plist_for_each: 按优先级从高到低遍历
	 */
	pr_info("  Iterate by priority (high to low):\n");
	plist_for_each_entry(pos, &head, node) {
		pr_info("    id=%d, prio=%d, value=%d\n",
			pos->id, pos->node.prio, pos->value);
	}

	/*
	 * plist_del: O(1) 删除
	 */
	my_plist_del(&d2->node, &head);
	test_assert(plist_node_empty(&d2->node), "deleted node is empty");

	/* 验证删除后顺序 */
	test_assert(plist_first(&head) == &d1->node, "after del, first still d1");
	test_assert(plist_last(&head) == &d3->node, "after del, last still d3");

	my_plist_del(&d1->node, &head);
	my_plist_del(&d3->node, &head);
	test_assert(plist_head_empty(&head), "list empty after all deleted");

	kfree(d1);
	kfree(d2);
	kfree(d3);
}

/* ============================================================
 * 第二层：同优先级 FIFO 行为
 * ============================================================ */

static void demo_same_prio_fifo(void)
{
	PLIST_HEAD(head);
	struct plist_data *d1, *d2, *d3, *pos;
	int count;
	int idx;
	/* plist 同优先级为 LIFO：后插入的排在前面 */
	int expected_ids[] = { 3, 2, 1 };

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 2: Same priority FIFO behavior\n");
	pr_info("================================================\n");

	/*
	 * 三个相同优先级的节点
	 * plist 保证同优先级按 FIFO 排序
	 */
	d1 = kmalloc(sizeof(*d1), GFP_KERNEL);
	d1->id = 1; d1->value = 100;
	plist_node_init(&d1->node, 50);

	d2 = kmalloc(sizeof(*d2), GFP_KERNEL);
	d2->id = 2; d2->value = 200;
	plist_node_init(&d2->node, 50);

	d3 = kmalloc(sizeof(*d3), GFP_KERNEL);
	d3->id = 3; d3->value = 300;
	plist_node_init(&d3->node, 50);

	my_plist_add(&d1->node, &head);
	my_plist_add(&d2->node, &head);
	my_plist_add(&d3->node, &head);

	/*
	 * 验证 FIFO 顺序：先插入的在前面
	 */
	pr_info("  Same prio (50) FIFO order:\n");
	count = 0;
	idx = 0;
	plist_for_each_entry(pos, &head, node) {
		pr_info("    id=%d (expected %d)\n", pos->id, expected_ids[idx]);
		test_assert(pos->id == expected_ids[idx],
			    "FIFO order correct");
		count++;
		idx++;
	}
	test_assert(count == 3, "all 3 nodes iterated");

	my_plist_del(&d1->node, &head);
	my_plist_del(&d2->node, &head);
	my_plist_del(&d3->node, &head);

	kfree(d1);
	kfree(d2);
	kfree(d3);
}

/* ============================================================
 * 第三层：混合优先级排序
 * ============================================================ */

static void demo_mixed_prio(void)
{
	PLIST_HEAD(head);
	struct plist_data *nodes[6];
	int prios[] = { 30, 10, 20, 10, 30, 20 };
	int ids[]    = { 1,  2,  3,  4,  5,  6 };
	/* LIFO for same prio: prio10(id4,id2) -> prio20(id3,id6) -> prio30(id1,id5) */
	int expected_order[] = { 4, 2, 3, 6, 1, 5 };
	int i;
	int idx = 0;
	struct plist_data *pos;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 3: Mixed priority sorting\n");
	pr_info("================================================\n");

	/* 创建 6 个节点，混合 3 种优先级 */
	for (i = 0; i < 6; i++) {
		nodes[i] = kmalloc(sizeof(*nodes[i]), GFP_KERNEL);
		nodes[i]->id = ids[i];
		nodes[i]->value = i;
		plist_node_init(&nodes[i]->node, prios[i]);
	}

	/* 乱序插入 */
	my_plist_add(&nodes[4]->node, &head);  /* prio 30, id 5 */
	my_plist_add(&nodes[1]->node, &head);  /* prio 10, id 2 */
	my_plist_add(&nodes[5]->node, &head);  /* prio 20, id 6 */
	my_plist_add(&nodes[0]->node, &head);  /* prio 30, id 1 */
	my_plist_add(&nodes[3]->node, &head);  /* prio 10, id 4 */
	my_plist_add(&nodes[2]->node, &head);  /* prio 20, id 3 */

	/*
	 * 验证排序：优先级从高到低，同优先级 FIFO
	 */
	pr_info("  Expected: prio10(id4,id2) -> prio20(id3,id6) -> prio30(id1,id5) (LIFO)\n");
	pr_info("  Actual:\n");
	plist_for_each_entry(pos, &head, node) {
		pr_info("    id=%d, prio=%d (expected id=%d)\n",
			pos->id, pos->node.prio, expected_order[idx]);
		test_assert(pos->id == expected_order[idx], "order correct");
		idx++;
	}
	test_assert(idx == 6, "all 6 nodes iterated in order");

	for (i = 0; i < 6; i++)
		my_plist_del(&nodes[i]->node, &head);

	for (i = 0; i < 6; i++)
		kfree(nodes[i]);
}

/* ============================================================
 * 第四层：plist_requeue（动态修改优先级）
 * ============================================================ */

static void demo_requeue(void)
{
	PLIST_HEAD(head);
	struct plist_data *d1, *d2, *d3, *pos;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 4: plist_requeue (change priority)\n");
	pr_info("================================================\n");

	d1 = kmalloc(sizeof(*d1), GFP_KERNEL);
	d1->id = 1; d1->value = 100;
	plist_node_init(&d1->node, 10);

	d2 = kmalloc(sizeof(*d2), GFP_KERNEL);
	d2->id = 2; d2->value = 200;
	plist_node_init(&d2->node, 20);

	d3 = kmalloc(sizeof(*d3), GFP_KERNEL);
	d3->id = 3; d3->value = 300;
	plist_node_init(&d3->node, 30);

	my_plist_add(&d1->node, &head);
	my_plist_add(&d2->node, &head);
	my_plist_add(&d3->node, &head);

	/* 初始顺序: d1(10) > d2(20) > d3(30) */
	test_assert(plist_first(&head) == &d1->node, "initially d1 first");

	/*
	 * plist_requeue: 修改节点优先级并重新排序
	 * 将 d1 从 prio 10 改为 prio 40（最低）
	 */
	d1->node.prio = 40;
	my_plist_requeue(&d1->node, &head);

	/* 新顺序: d2(20) > d3(30) > d1(40) */
	test_assert(plist_first(&head) == &d2->node, "after requeue, d2 is first");
	test_assert(plist_last(&head) == &d1->node, "after requeue, d1 is last");

	pr_info("  After requeue d1: prio 10 -> 40\n");
	plist_for_each_entry(pos, &head, node) {
		pr_info("    id=%d, prio=%d\n", pos->id, pos->node.prio);
	}

	/* 将 d1 改回最高优先级 */
	d1->node.prio = 5;
	my_plist_requeue(&d1->node, &head);

	test_assert(plist_first(&head) == &d1->node, "after requeue back, d1 first again");

	my_plist_del(&d1->node, &head);
	my_plist_del(&d2->node, &head);
	my_plist_del(&d3->node, &head);

	kfree(d1);
	kfree(d2);
	kfree(d3);
}

/* ============================================================
 * 第五层：first/next 访问器
 * ============================================================ */

static void demo_accessors(void)
{
	PLIST_HEAD(head);
	struct plist_data *d1, *d2, *d3, *entry;
	struct plist_node *first_node;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 5: Accessors (first/last/next)\n");
	pr_info("================================================\n");

	d1 = kmalloc(sizeof(*d1), GFP_KERNEL);
	d1->id = 1; d1->value = 100;
	plist_node_init(&d1->node, 10);

	d2 = kmalloc(sizeof(*d2), GFP_KERNEL);
	d2->id = 2; d2->value = 200;
	plist_node_init(&d2->node, 20);

	d3 = kmalloc(sizeof(*d3), GFP_KERNEL);
	d3->id = 3; d3->value = 300;
	plist_node_init(&d3->node, 30);

	my_plist_add(&d1->node, &head);
	my_plist_add(&d2->node, &head);
	my_plist_add(&d3->node, &head);

	/*
	 * plist_first_entry: 获取第一个 entry（最高优先级）
	 */
	entry = plist_first_entry(&head, struct plist_data, node);
	test_assert(entry == d1, "first_entry returns d1");

	/*
	 * plist_last_entry: 获取最后一个 entry（最低优先级）
	 */
	entry = plist_last_entry(&head, struct plist_data, node);
	test_assert(entry == d3, "last_entry returns d3");

	/*
	 * plist_next / plist_prev: 获取前驱/后继
	 */
	first_node = plist_next(&d1->node);
	test_assert(first_node == &d2->node, "next(d1) = d2");

	first_node = plist_prev(&d3->node);
	test_assert(first_node == &d2->node, "prev(d3) = d2");

	my_plist_del(&d1->node, &head);
	my_plist_del(&d2->node, &head);
	my_plist_del(&d3->node, &head);

	kfree(d1);
	kfree(d2);
	kfree(d3);
}

/* ============================================================
 * 第六层：真实场景模拟（RT 调度器 pushable_tasks）
 * ============================================================ */

static void demo_rt_sched_pushable(void)
{
	/*
	 * 模拟内核 RT 调度器的 pushable_tasks：
	 * - 每个 RT task 有一个 pushable_node
	 * - 按优先级排序，便于快速找到最高优先级的可推送任务
	 * - 当 task 变为 RT 时加入，不再是 RT 时移除
	 */
	PLIST_HEAD(pushable_tasks);
	struct plist_data *tasks[5];
	int init_prios[] = { 50, 30, 70, 10, 90 };
	int task_ids[] = { 100, 200, 300, 400, 500 };
	int i;
	struct plist_data *pos;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("    Layer 6: RT sched pushable_tasks sim\n");
	pr_info("================================================\n");

	/* 初始化 5 个 RT task */
	for (i = 0; i < 5; i++) {
		tasks[i] = kmalloc(sizeof(*tasks[i]), GFP_KERNEL);
		tasks[i]->id = task_ids[i];
		tasks[i]->value = init_prios[i];
		plist_node_init(&tasks[i]->node, init_prios[i]);
	}

	/*
	 * Task 变为 RT，加入 pushable_tasks
	 */
	pr_info("  Tasks become RT, adding to pushable_tasks:\n");
	for (i = 0; i < 5; i++) {
		my_plist_add(&tasks[i]->node, &pushable_tasks);
		pr_info("    task id=%d, prio=%d\n", tasks[i]->id, tasks[i]->node.prio);
	}

	/*
	 * 获取最高优先级的可推送任务
	 */
	pos = plist_first_entry(&pushable_tasks, struct plist_data, node);
	pr_info("  Highest prio pushable task: id=%d, prio=%d\n",
		pos->id, pos->node.prio);
	test_assert(pos->id == 400, "highest prio task is id=400 (prio=10)");

	/*
	 * 模拟：task 300 的动态优先级变化
	 */
	pr_info("  Task 300 priority change: 70 -> 5\n");
	tasks[2]->node.prio = 5;
	my_plist_requeue(&tasks[2]->node, &pushable_tasks);

	pos = plist_first_entry(&pushable_tasks, struct plist_data, node);
	test_assert(pos->id == 300, "after requeue, task 300 is highest prio");

	/*
	 * 遍历所有可推送任务（按优先级）
	 */
	pr_info("  All pushable tasks (prio order):\n");
	plist_for_each_entry(pos, &pushable_tasks, node) {
		pr_info("    task id=%d, prio=%d\n", pos->id, pos->node.prio);
	}

	/*
	 * Task 不再是 RT，从链表移除
	 */
	pr_info("  Tasks leave RT, removing from pushable_tasks:\n");
	for (i = 0; i < 5; i++) {
		my_plist_del(&tasks[i]->node, &pushable_tasks);
		kfree(tasks[i]);
	}
	test_assert(plist_head_empty(&pushable_tasks), "all tasks removed");
}

/* ============================================================
 * 模块入口 / 出口
 * ============================================================ */

static int __init plist_test_init(void)
{
	g_passed = 0;
	g_failed = 0;

	pr_info("\n");
	pr_info("================================================\n");
	pr_info("       plist demo module loaded\n");
	pr_info("================================================\n");

	demo_basic_ops();
	demo_same_prio_fifo();
	demo_mixed_prio();
	demo_requeue();
	demo_accessors();
	demo_rt_sched_pushable();

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

static void __exit plist_test_exit(void)
{
	pr_info("plist_test module exit, bye!\n");
}

module_init(plist_test_init);
module_exit(plist_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629");
MODULE_DESCRIPTION("plist demo module");
