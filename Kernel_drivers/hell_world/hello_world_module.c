#include <Linux/module.h>
#include <Linux/init.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rokas Barasa GNU/Linux");
MODULE_DESCRIPTION("Hello world LKM"); // LKM is linux kernel module

// Called when it is loaded into the kernel
static int __init ModuleInit(void){
    printk("Hello, Kernel!\n");
    return 0;
}

// Called when this module is removed from kernel
static void __exit ModuleExit(void){
    printk("Goodbye, Kernel!\n");

}