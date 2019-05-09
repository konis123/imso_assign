KDIR := ~/linux-rpi
ARM := ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-
PWD := $(shell pwd)

obj-m := ku_pir.o

default:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) modules

copy:
	arm-linux-gnueabi-gcc -c ku_pir_lib.c
	arm-linux-gnueabi-gcc -I. -o reader ku_pir_app_read.c ku_pir_lib.o
	scp mknod.sh ku_pir.ko reader pi@10.10.10.13:~

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $(ARM) clean
