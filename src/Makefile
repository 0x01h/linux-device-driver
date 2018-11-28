obj-m+=mydriver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

# $ make test
test:	
	sudo dmesg -C
	sudo insmod mydriver.ko
	sudo mknod /dev/mydriver0 c 238 0
	sudo mknod /dev/mydriver1 c 238 1
	echo 'test mydriver0' > /dev/mydriver0
	cat /dev/mydriver0
	echo 'test mydriver1' > /dev/mydriver1
	cat /dev/mydriver1
	sudo rmmod mydriver.ko
	sudo dmesg