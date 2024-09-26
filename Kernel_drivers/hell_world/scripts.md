sudo insmod hello_world_module.ko // load the module into the kernel
lsmod | grep hello_world_module // list modules to see if it is there
dmesg | tail // Kernel log, all of the kernel not just the module. But it could show the output of our module
sudo rmmod hello_world_module // Remove module 
dmesg | tail // Check log again to see the output
