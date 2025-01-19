My Raspberry pi 4 2Gb had problems with this command: sudo apt-get install linux-headers-$(uname -r) The problem was that even if the 32 bit raspberry pi operating system is installed it is still trying to use the 64 bit kernel or something. I needed edit"/boot/firmware/config.txt" and in the "ALL" section add "arm_64bit=0". After reboot it the command worked fine.

sudo apt-get install linux-headers-$(uname -r)
sudo apt update && sudo apt upgrade -y

sudo insmod hello_world_module.ko                  // load the module into the kernel
lsmod | grep hello_world_module                    // list modules to see if it is there
dmesg | tail                                       // Kernel log, all of the kernel not just the module. But it could show the output of our module
sudo rmmod hello_world_module                      // Remove module 
dmesg | tail                                       // Check log again to see the output

# device numbers 

Device numbers are used to link files to kernel drivers. A Kernel driver initializes itself as a device driver in "/proc/devices" then a kernel driver can create a device file in "/dev" with its major and minor versions to link the device driver to the device file. The kernel driver then gets events if the file in "/dev" is interacted with.


To help the driver interact with the user space or other kernel drivers, a kernel driver can have a device number created for it. This gives it an id for other things to attach to it or interact with it. 


ls -al /dev/tty*
cat /proc/devices
cat /proc/devices | grep device_number

sudo mknod /dev/mydevice c 90 0
ls /dev/mydevice -al