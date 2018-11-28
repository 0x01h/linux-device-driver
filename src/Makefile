obj-m+=mydriver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

# $ make test
test:	
	sudo dmesg -C
	sudo insmod mydriver.ko
	sudo rmmod mydriver.ko
	sudo dmesg