#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ku_ipc.h"

#define DEV_NAME "assign_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4

#define ASSIGN_IOCTL_NUM 'z'
#define ASSIGN_IOCTL1 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define ASSIGN_IOCTL2 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define ASSIGN_IOCTL3 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define ASSIGN_IOCTL4 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM4, unsigned long *)

struct msgbuf{
	long type;
	char text[100];
	//char* text;
};

int ku_msgget(int key, int msgflg);
int ku_msgclose(int msqid);
int ku_msgsnd(int msqid, void *msgp, int msgsz, int msgflg);
int ku_msgrcv(int msqid, void *msgp, int msgsz, long msgtyp, int msgflg);

int ku_msgget(int key, int msgflg){
	int dev;
	ku_msgget_param value = {key, msgflg};
	dev = open("/dev/assign_dev", O_RDWR);

	int temp = ioctl(dev, ASSIGN_IOCTL1, &value);

	close(dev);

	return temp;
}

int ku_msgclose(int msqid){
	int dev;
	ku_msgclose_param value = {msqid};
	dev = open("/dev/assign_dev", O_RDWR);

	int temp = ioctl(dev, ASSIGN_IOCTL2, &value);

	close(dev);

	return temp;
}

int ku_msgsnd(int msqid, void *msgp, int msgsz, int msgflg){
	int dev;
	ku_msgsnd_param value = {msqid, msgp, msgsz, msgflg};
	dev = open("/dev/assign_dev", O_RDWR);
	
	ioctl(dev, ASSIGN_IOCTL3, &value);
	
	close(dev);
}

int ku_msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg){
	int dev;
	struct msgbuf *tmp;
	int ret=0;
	ku_msgrcv_param value = {msqid, msgp, msgsz, msgtyp, msgflg};
	dev = open("/dev/assign_dev", O_RDWR);

	ioctl(dev, ASSIGN_IOCTL4, &value);

	tmp = (struct msgbuf*)value.msgp;
	ret = strlen(tmp->text);

	printf("msgrcv : %s\n",tmp->text);
	
	
	close(dev);
	return ret;
}

int main(int argc, char* argv[]){

	int qid = ku_msgget(4, KU_IPC_CREAT);
	//int qid2 = ku_msgget(5, KU_IPC_EXCL);
	struct msgbuf s1 = {51, "hey"};
	//struct msgbuf s2 = {52, "hello"};
	//struct msgbuf s3 = {53, "nihao"};
	struct msgbuf s4 = {51, "goodnight"};
	struct msgbuf s5 = {51, "first"};
	struct msgbuf s6 = {51, "second"};
	struct msgbuf s7 = {51, "third"};
	struct msgbuf s8 = {51, "fourth"};


	struct msgbuf r1,r2,r3,r4,r5,r6,r7;

	if(argc == 3){//rcv
		ku_msgrcv(qid, &r3, sizeof(r4), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );
		printf("receive\n");
	}else if(argc == 4){//test
		printf("print test\n");
	}else if(argc == 2){//send
		printf("send\n");
		ku_msgsnd(qid, &s4, sizeof(s4), 0);
	}else{
		ku_msgsnd(qid, &s1, sizeof(s1), KU_IPC_NOWAIT);//hey
		//ku_msgsnd(qid, &s2, sizeof(s2), KU_IPC_NOWAIT);//hello
		//ku_msgsnd(qid2, &s3, sizeof(s3), KU_IPC_NOWAIT);//nihao
		ku_msgsnd(qid, &s4, sizeof(s4), KU_IPC_NOWAIT);//goodnight
		ku_msgsnd(qid, &s5, sizeof(s5), KU_IPC_NOWAIT);
		ku_msgsnd(qid, &s6, sizeof(s6), KU_IPC_NOWAIT);
		ku_msgsnd(qid, &s7, sizeof(s7), KU_IPC_NOWAIT);
		ku_msgsnd(qid, &s8, sizeof(s8), 0);
	
	
		//ku_msgrcv(qid2, &r1, sizeof(r1), 53, (KU_IPC_NOWAIT|KU_MSG_NOERROR));//nihao
		ku_msgrcv(qid, &r2, sizeof(r2), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );//hey
		ku_msgrcv(qid, &r3, sizeof(r3), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );//goodnight
		ku_msgrcv(qid, &r3, sizeof(r4), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );
		ku_msgrcv(qid, &r3, sizeof(r5), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );
		ku_msgrcv(qid, &r3, sizeof(r6), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );
		ku_msgrcv(qid, &r3, sizeof(r7), 51, (KU_IPC_NOWAIT|KU_MSG_NOERROR) );
		

		//printf("---rcv data:%s, %s, %s---\n", r1.text, r2.text, r3.text);
	
		//ku_msgclose(qid);
		//ku_msgclose(qid2);
	}

	//ku_msgclose(qid);

	return 0;
}
