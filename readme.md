# Writing various kernel drivers on embedded linux RP4 and trying out building a custom OS using buildroot and yocto



## Kernel drivers
Kernel driver writing happens on the current computer. It does not need to be the Linux one. To get into the RPI using windows command line, use "ssh pi@<IP_OF_RPI>" then the pasword is just "pi". Dont use Putty.

The kernel drivers are stored in "/home/pi/Desktop". Just "cd Desktop" once you loggin.

I think the OS i am using is a Raspberry PI OS 64 bit.

Some non tutorial drivers:
* oled_driver

## OS building
For OS building I need to VNC into my old linux laptop. There i have pulled down buildroot, which i did a test build for with success, and yocto, which i tried to build unsucesfully.

