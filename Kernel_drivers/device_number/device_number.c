#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rokas Barasa GNU/Linux");
MODULE_DESCRIPTION("Register a device nr. and implement some callback functions"); // LKM is linux kernel module


static int driver_open(struct inode *device_file, struct file * instance){
    printk("Dev nr - open was called!\n");
    return 0;
}

static int driver_close(struct inode *device_file, struct file * instance){
    printk("Dev nr - close was called!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close
};

#define MYMAJOR 90

// Called when it is loaded into the kernel
static int __init ModuleInit(void){
    printk("Hello, Kernel DEVICE!\n");
    // Register device number
    int retval = register_chrdev(MYMAJOR, "device_number", &fops);
    if(retval == 0){
        printk("device_number - registed device number major: %d, minor: %d\n", MYMAJOR, 0);
    }else if(retval > 0){
        printk("device_number - registed device number major: %d, minor: %d\n", retval>>20, retval&0xfffff);
    }else{
        printk("Could not register device number\n");
        return -1;
    }

    return 0;
}

// Called when this module is removed from kernel
static void __exit ModuleExit(void){
    unregister_chrdev(MYMAJOR, "device_number");
    printk("Goodbye, Kernel DEVICE!\n");
}

module_init(ModuleInit)
module_exit(ModuleExit)