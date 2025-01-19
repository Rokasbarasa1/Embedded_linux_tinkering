
The gpio pins are offset by some large ammount use this to find the mapping;
cat /sys/kernel/debug/gpio

sudo insmod gpio_driver.ko
dmesg
ls /dev                 // my_gpio_driver