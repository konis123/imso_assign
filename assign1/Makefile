KDIR := ~/linux-rpi
ARM := ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
PWD := $(shell pwd)

obj-m := ku_ipc.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules

copy:
	scp ku_ipc.ko pi@10.10.10.13:~

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) clean
