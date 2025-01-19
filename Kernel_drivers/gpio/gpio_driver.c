#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>


/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rokas Barasa GNU/Linux");
MODULE_DESCRIPTION("A simple driver for setting a led and reading a button");

// Variables for device number and device file
static dev_t my_device_nr;
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "my_gpio_driver"
#define DRIVER_CLASS "MyModuleClass"

// callback read
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;
    char tmp[3] = " \n";


	/* Get amount of data to copy */
	to_copy = min_t(size_t, count, sizeof(tmp));

    // Read value of button
    printk("Value of button: %d\n", gpio_get_value(529));
    tmp[0] =  gpio_get_value(529) + '0';

    // Copy to user
    not_copied = copy_to_user(user_buffer, &tmp, to_copy);

    // Calculate data
    delta = to_copy - not_copied;

    return delta;
}

// callback write
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;
    char value;

    // Get amount of data to copy
    to_copy = min_t(size_t, count, sizeof(value));

    // Copy from user
    not_copied = copy_from_user(&value, user_buffer, to_copy);

    // Setting the LED
    switch(value){
        case '0':
            gpio_set_value(516, 0);
            break;
        case '1': 
            gpio_set_value(516, 1);
            break;
        default:
            printk("Invalid input\n");
            break;
    }

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
    if (IS_ERR(my_class)) {
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


    // GPIO functionality ----------------------------------------------------

    // Init GPIO 4 (516)
    if(gpio_request(516, "rpi-gpio-516")){
        printk("Cannot allocate GPIO 516\n");
        goto AddError;
    }
    if(gpio_direction_output(516, 0)){
        printk("Can not set GPIO 516 to output\n");
        goto Gpio516Error;
    }

    // Init GPIO 17 (529)
    if(gpio_request(529, "rpi-gpio-529")){
        printk("Cannot allocate GPIO 529\n");
        goto AddError;
    }
    if(gpio_direction_input(529)){
        printk("Can not set GPIO 529 to input\n");
        goto Gpio529Error;
    }


    return 0;

Gpio529Error:
	gpio_free(529);
Gpio516Error:
	gpio_free(516);
AddError:
	device_destroy(my_class, my_device_nr);
FileError:
	class_destroy(my_class);
ClassError:
	unregister_chrdev_region(my_device_nr, 1);
	return -1;
}

// Called when this module is removed from the kernel
static void __exit ModuleExit(void) {

    gpio_set_value(516, 0);
    gpio_free(516);
    gpio_free(529);


    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev(my_device_nr, DRIVER_NAME);
    printk("Goodbye, Kernel DEVICE!\n");
}

module_init(ModuleInit)
module_exit(ModuleExit)