

enable the i2c:
sudo raspi-config

can also be done trough:
sudo nano /boot/firmware/config.txt

Find modules connected to it:
sudo i2cdetect -y 1





sudo insmod oled_driver.ko
sudo rmmod oled_driver


sudo chmod 666 /dev/oled_driver
echo "Hello world!" > /dev/oled_driver