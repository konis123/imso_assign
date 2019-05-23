#include <stdio.h>
#include <sys/fcntl.h>

void main(void){

	int dev;
	char c;
	dev = open("/dev/c4_dev", O_RDWR);
	scanf("%c", &c);
	close(dev);
}
