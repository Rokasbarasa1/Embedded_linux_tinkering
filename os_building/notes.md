Clean the PATH 
export PATH=$(echo "$PATH" | tr -s ':' '\n' | grep -v ' ' | tr -s '\n' ':')

Password when connecting to shitty linux laptop trough VNC password:
uogauoga123 (truncated to 8 characters)

Buildroot default login password: root

Yocto releases
https://wiki.yoctoproject.org/wiki/Releases

Raspberry pi board support for yocto
https://github.com/agherzan/meta-raspberrypi

Find other board support packages:
https://layers.openembedded.org/layerindex/branch/master/layers/

COnnect to raspberry pi trough ftdi
picocom -b 115200 /dev/ttyUSB0


source poky/oe-init-build-env rpi-build

I donwload all the needed packages for my board. I found the meta-raspberrypi pacakage and luckily it did not have any dependencies besides poky. I installed both in the yocto folder. i then used: 
"bitbake-layers show-layers" 
to see the layers i have. To that i add the board support layer. To add it i edit the conf/bblayers.conf file.

I am apparently building for the qemu emulator. Bitbake is apparently by default building for it.
In build rpi i can select the machine to build for in local.conf.

MACHINE = "raspberrypi4-64.conf"
XZ_DEFAULTS = "--memlimit=1500MiB"

Need to tell bitbake what image to build: core-image-minimal

can be found here https://docs.yoctoproject.org/ref-manual/images.html

I will use core-image-minimal-dev because i want to put linux kernel stuff in it

To make some changes before building using bitbake run:
bitbake -c menuconfig virtual/kernel

to build it use:
bitbake core-image-minimal-dev



