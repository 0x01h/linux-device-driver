obj-m+=mydriver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

# $ make test
test:	
	sudo dmesg -C
	sudo insmod mydriver.ko
	sudo mknod /dev/queue0 c 238 0
	sudo mknod /dev/queue1 c 238 1
	echo 'test queue0' > /dev/queue0
	cat /dev/queue0
	echo 'test queue1' > /dev/queue1
	cat /dev/queue1
	sudo rmmod mydriver.ko
	sudo dmesg | grep 'My Driver'