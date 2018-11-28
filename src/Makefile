obj-m+=mydriver.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

# $ make test
test:	
	dmesg -C
	insmod mydriver.ko
	rmmod mydriver.ko
	dmesg