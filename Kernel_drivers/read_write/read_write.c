#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rokas Barasa GNU/Linux");
MODULE_DESCRIPTION("Register a device nr. and implement some callback functions");

// Buffer for data
static char buffer[255];
static int buffer_pointer;

static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "dummydriver"
#define DRIVER_CLASS "MyModuleClass"

// callback read
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;

    // Get amount of data to copy
    to_copy = min_t(size_t, count, buffer_pointer);

    // Copy to user
    not_copied = copy_to_user(user_buffer, buffer, to_copy);

    // Calculate data
    delta = to_copy - not_copied;

    return delta;
}

// callback write
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;

    // Get amount of data to copy
    to_copy = min_t(size_t, count, sizeof(buffer));

    // Copy from user
    not_copied = copy_from_user(buffer, user_buffer, to_copy);
    buffer_pointer = to_copy;

    // Calculate data
    delta = to_copy - not_copied;

    return delta;
}

static int driver_open(struct inode *device_file, struct file * instance) {
    printk("Dev nr - open was called!\n");
    return 0;
}

static int driver_close(struct inode *device_file, struct file * instance) {
    printk("Dev nr - close was called!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};

#define MYMAJOR 90

// Called when it is loaded into the kernel
static int __init ModuleInit(void) {
    printk("Hello, Kernel DEVICE!\n");

    // Get a device number value
    if (alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0) {
        printk("Device NR. could not be allocated\n");
        return -1;
    }

    printk("read_write - device nr major %d minor %d was registered!\n", MAJOR(my_device_nr), MINOR(my_device_nr));

    // Create device class
    my_class = class_create(DRIVER_CLASS);
    if (my_class == NULL) {
        printk("Device class cannot be created!\n");
        goto ClassError;
    }

    // Create device file
    if (device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) {
        printk("Cannot create device file!\n");
        goto FileError;
    }

    // Initialize device file
    cdev_init(&my_device, &fops);

    // Add device to the system
    if (cdev_add(&my_device, my_device_nr, 1) == -1) {
        printk("Register of device through kernel failed\n");
        goto AddError;
    }

    return 0;

AddError:
    device_destroy(my_class, my_device_nr);
    return -1;

FileError:
    class_destroy(my_class);
    return -1;

ClassError:
    unregister_chrdev(my_device_nr, DRIVER_NAME);
    return -1;
}

// Called when this module is removed from the kernel
static void __exit ModuleExit(void) {
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev(my_device_nr, DRIVER_NAME);
    printk("Goodbye, Kernel DEVICE!\n");
}

module_init(ModuleInit)
module_exit(ModuleExit)