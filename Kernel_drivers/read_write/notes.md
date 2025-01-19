sudo insmod read_write.ko
sudo rmmod read_write
lsmod | grep read_write
dmesg | tail
ls /dev
sudo chmod 666 /dev/dummydriver
echo "This is a test" > /dev/dummydriver
head -n 1 /dev/dummydriver