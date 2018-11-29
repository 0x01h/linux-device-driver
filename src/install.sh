# Major number of the driver could be different on other computers, check it from DMESG change it to make node manually.
# MAJOR_NUM=235
sudo rmmod mydriver
make
sudo dmesg -C
sudo insmod ./mydriver.ko driver_nr_devs=20
# sudo mknod /dev/queue0 c $MAJOR_NUM 0
sudo echo "test queue0" > /dev/queue0
sudo cat /dev/queue0
sudo echo "test queue3" > /dev/queue3
sudo cat /dev/queue3
dmesg | grep 'My Driver:'
