// #include <linux/module.h>
// #include <linux/init.h>
// #include <linux/gpio.h>
// #include <linux/interrupt.h>
// #include <linux/irq.h>

// #define TEST_GPIO_PIN 101

// static int irq;

// void testsoftirq_func(struct softirq_action *softirq_act)
// {
//         pr_info("testsoftirq_func is called\n");
// }

// irqreturn_t test_softirq_handler(int irq, void *dev_id)
// {
//         pr_info("test_softirq_handler is called\n");
//         raise_softirq(TASKLET_SOFTIRQ);
//         return IRQ_RETVAL(IRQ_HANDLED);
// }

// static int __init softirq_test_init(void)
// {
//         int ret;
//         irq = gpio_to_irq(TEST_GPIO_PIN);
//         if (irq < 0)
//                 return -ENODEV;

//         ret = request_irq(irq, test_softirq_handler, IRQF_TRIGGER_RISING, "test", NULL);
//         if (ret < 0)
//                 return -ENODEV;

//         open_softirq(TASKLET_SOFTIRQ, testsoftirq_func);

//         return 0;
// }

// static void __exit softirq_test_exit(void)
// {
//         free_irq(irq, NULL);
// }

// module_init(softirq_test_init);
// module_exit(softirq_test_exit);

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("even629<asqwgo@outlook.com>");
// MODULE_DESCRIPTION("This is test sample for softirq");
