obj-m+=mydriver.o

all:
	sudo dmesg -C
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

# $ make test
test:
	sudo rmmod mydriver # If old one was already loaded.	
	sudo dmesg -C		   # Clear all previous kernel messages.
	sudo insmod ./mydriver.ko driver_nr_devs=20	# Create 20 device nodes.
	# sudo mknod /dev/queue0 c 238 0
	# sudo mknod /dev/queue1 c 238 1
	echo 'test queue0' > /dev/queue0
	cat /dev/queue0
	echo 'test queue1' > /dev/queue1
	cat /dev/queue1
	sudo rmmod mydriver
	sudo dmesg | grep 'My Driver'