#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>

// 定义一个 i2c_adapter 结构体指针
struct i2c_adapter *i2c_ada;

// 定义 i2c_board_info 结构体数组，用于描述 ft5x06 设备

static struct i2c_board_info ft5x06[] = {
        {
                .type = "my-ft5x06",
                .addr = 0x38,
        }
};



static int __init i2c_client_test_init(void){
        // 获取 i2c 适配器
        i2c_ada = i2c_get_adapter(1);
        if(!i2c_ada){
                pr_err("Fail to get i2c_adapter1");
                return -ENODEV;
        }
        // 注册 ft5x06 设备
        i2c_new_client_device(i2c_ada, ft5x06);        
        
        return 0;
}


static void __exit i2c_client_test_exit(void){
        // 释放 i2c 适配器
        i2c_put_adapter(i2c_ada);
}

module_init(i2c_client_test_init);
module_exit(i2c_client_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("even629 <asqwgo@outlook.com>");
MODULE_DESCRIPTION("This is a test sample for i2c client");
