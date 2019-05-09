#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ku_pir.h"


#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4
#define IOCTL_NUM5 IOCTL_START_NUM+5

#define ASSIGN_IOCTL_NUM 'z'
#define ASSIGN_IOCTL1 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define ASSIGN_IOCTL2 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define ASSIGN_IOCTL3 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define ASSIGN_IOCTL4 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM4, unsigned long *)
#define ASSIGN_IOCTL5 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM5, unsigned long *)

int ku_pir_open();
int ku_pir_close(int fd);
void ku_pir_read(int fd, struct ku_pir_data *data);
void ku_pir_flush(int fd);
int ku_pir_insertData(long unsigned int ts, char rf_flag);

static int dev;

int ku_pir_open(){
	//int dev;
	dev = open("/dev/ku_pir_dev", O_RDWR);
	if(dev <= 0)	return dev;

	//printf("open:%d(this is fd)\n", dev);
	int temp = ioctl(dev, ASSIGN_IOCTL1, dev);
	//printf("ioctl:%d\n", temp);

	return dev;
}

int ku_pir_close(int fd){
	int temp = ioctl(fd, ASSIGN_IOCTL2, fd);
	//printf("close temp:%d\n", temp);
	if(temp == 0){//temp==0이면 해당 fd의 자료구조가 있는거.
		close(fd);
		printf("fd:%d close\n", fd);
		return 0;
	}else{//temp!=0이면 해당 fd의 자료구조가 없는거.
		printf("fd is not valid\n");
		return -1;
	}

}

void ku_pir_read(int fd, struct ku_pir_data *data){
	//int dev;
	struct ku_pir_data *tmp;
	struct ku_pir_read_param value = {fd, data};
	//dev = open("/dev/ku_pir_dev", O_RDWR);
	
	ioctl(fd, ASSIGN_IOCTL3, &value);
	
	tmp = (struct ku_pir_data*)value.data;//이거 잘못됬을수있음 현재.
	printf("read %ld, %c\n", tmp->timestamp, tmp->rf_flag);

	//close(dev);
}

void ku_pir_flush(int fd){
	//int dev;
	int value = fd;
	//dev = open("/dev/ku_pir_dev", O_RDWR);

	ioctl(fd, ASSIGN_IOCTL4, &value);
	
	//close(dev);
}

int ku_pir_insertData(long unsigned int ts, char rf_flag){
	//int dev;
	struct ku_pir_data tmpData = {ts, rf_flag};
	struct ku_pir_insertData_param value = {tmpData};
	//dev = open("/dev/ku_pir_dev", O_RDWR);

	ioctl(dev, ASSIGN_IOCTL5, &value);

	printf("insertData %ld, %d\n", ts, rf_flag);

	//close(dev);
}
/*
int main(int argc, char* argv[]){

	int c;

	struct ku_pir_data rcvData1;
	struct ku_pir_data rcvData2;

	int t1 = ku_pir_open();
	int t2 = ku_pir_open();

	ku_pir_insertData(1000,1);
	ku_pir_insertData(1234, 2);

	ku_pir_read(t1, &rcvData1);

	printf("rcvData.ts:%ld rcvData.rf_flag:%d\n", rcvData1.timestamp, rcvData1.rf_flag);
	scanf("%c", &c);


	ku_pir_close(t1);
	ku_pir_close(t2);

	return 0;
}*/
