sudo rmmod mydriver
sudo rm /dev/queue0
make
sudo dmesg -C
sudo insmod ./mydriver.ko
sudo mknod /dev/queue0 c 238 0
sudo echo "test" > /dev/queue0
sudo cat /dev/queue0
dmesg | grep 'My Driver:'
