# 238 is the major number of the driver could be different on other computers, check it from DMESG.
MAJOR_NUM=238
sudo rmmod mydriver
sudo rm /dev/queue0
make
sudo dmesg -C
sudo insmod ./mydriver.ko
sudo mknod /dev/queue0 c $MAJOR_NUM 0
sudo echo "test" > /dev/queue0
sudo cat /dev/queue0
dmesg | grep 'My Driver:'
