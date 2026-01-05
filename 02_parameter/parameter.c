#include <linux/init.h>
#include <linux/module.h>


static int myint = 0;
module_param(myint,int, 0644);
MODULE_PARM_DESC(myint, "A sample int parameter");

static char* mycharp = "hello";
module_param(mycharp, charp, 0644);
MODULE_PARM_DESC(mycharp, "A sample charp parameter");

static int myarr[3] = {1, 2, 3};
static int myarr_argc = ARRAY_SIZE(myarr);
module_param_array(myarr, int, &myarr_argc, 0644);
MODULE_PARM_DESC(myarr, "A sample array parameter");

static char mystring[] = "default_value";
module_param_string(mystr, mystring, ARRAY_SIZE(mystring), 0644);
MODULE_PARM_DESC(mystr, "A sample string parameter");


static void print_param(void){
        int i;
        pr_info("[myint]: %d\n", myint);
        pr_info("[mycharp]: %s\n", mycharp);
        pr_info("[myarr]: ");
        for(i =0;i<myarr_argc;i++){
                pr_info("%d ", myarr[i]);
        }              
        pr_info("[mystr]: %s\n", mystring);
}

static int __init param_test_init(void){        
        printk("param test init\n");
        print_param();
        return 0;
}

static void __exit param_test_exit(void){
        print_param();
        printk("param test exit\n");
}

module_init(param_test_init);
module_exit(param_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629<asqwgo@163.com>");
MODULE_DESCRIPTION("linux driver parameter test");
