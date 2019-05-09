#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include "ku_ipc.h"

#define DEV_NAME "assign_dev"

#define IOCTL_START_NUM 0x80
#define IOCTL_NUM1 IOCTL_START_NUM+1
#define IOCTL_NUM2 IOCTL_START_NUM+2
#define IOCTL_NUM3 IOCTL_START_NUM+3
#define IOCTL_NUM4 IOCTL_START_NUM+4
//#define IOCTL_NUM5 IOCTL_START_NUM+5

#define ASSIGN_IOCTL_NUM 'z'
#define ASSIGN_IOCTL1 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM1, unsigned long *)
#define ASSIGN_IOCTL2 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM2, unsigned long *)
#define ASSIGN_IOCTL3 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM3, unsigned long *)
#define ASSIGN_IOCTL4 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM4, unsigned long *)
//#define ASSIGN_IOCTL5 _IOWR(ASSIGN_IOCTL_NUM, IOCTL_NUM5, unsigned long *)

spinlock_t my_lock;

struct msgbuf{
	long type;
	char text[100];
};

struct queue_node{
	struct list_head head;
	struct msgbuf buf;
	int msgsize;
	int msgvol;
};

struct assign_node{
	struct list_head head;
	struct queue_node qList;
	int qid;
	int qsize;
};

struct assign_node mylist;

//unsigned int k = 0;
int keyArray[10000];	//키%10000+10000 하면 qid

int isFull(int qid){
	int isfull = 0;
	struct assign_node *tmp = 0;
	struct queue_node *qtmp = 0;
	struct list_head *pos = 0;
	
	list_for_each(pos, &mylist.head){//큐를 돌면서 큐에메시지가 있는지 확인
                tmp = list_entry(pos, struct assign_node, head);
                if((tmp->qid) == qid){
			if( tmp->qList.msgsize >= KUIPC_MAXMSG || tmp->qList.msgvol >= KUIPC_MAXVOL ){
				isfull = 1;
			}else{
				isfull = 0;
			}
		}
	}

	return isfull;
}

static long assign_ioctl(struct file* file, unsigned int cmd, unsigned long arg){
	int key,msgflg,msqid,msgsz;
	long msgtyp;
	int ctu,cfu;
	struct msgbuf *msgp;
	int qid;
	bool isExist = false;
	struct assign_node *tmp = 0;
	struct queue_node *qtmp = 0;
       	struct list_head *pos = 0;
        struct list_head *q = 0;
	struct list_head *qpos = 0;
        unsigned int i = 0;
	//int k = 0;	//큐에서 지금 어디 인덱스를 가리키고있는지.

	switch(cmd){
		case ASSIGN_IOCTL1://리스트 init 해야할듯.
			key = (((ku_msgget_param*)arg)->key);
			msgflg = ((ku_msgget_param*)arg)->msgflg;
			if(msgflg == KU_IPC_CREAT && keyArray[key%10000] != 0){
				keyArray[key%10000]++;
				printk("creat && key is used keyArray[%d]:%d", key%10000, keyArray[key%10000]);
				return key%10000 + 10000;
			}else if(msgflg == KU_IPC_EXCL && keyArray[key%10000] != 0){
				printk("excl && key is used");
				return -1;
			}
			
			//printk("list make & return qid");
			qid = key%10000 + 10000;
			keyArray[key%10000]++;

			tmp = (struct assign_node*)kmalloc(sizeof(struct assign_node), GFP_KERNEL);
			INIT_LIST_HEAD(&tmp->qList.head);
			tmp->qid = qid;
			(tmp->qList).msgsize = 0;
			(tmp->qList).msgvol = 0;
                        list_add_tail(&tmp->head, &mylist.head);

			return qid;
		case ASSIGN_IOCTL2://여러개열렸을떄 수정할부분있음
			msqid = ((ku_msgclose_param*)arg)->msqid;
			
			if(keyArray[msqid-10000] == 0){
				printk("qid not exist");
				return -1;
			}

			keyArray[msqid-10000]--;
			if(keyArray[msqid-10000] == 0){
				spin_lock(&my_lock);
				list_for_each_safe(pos, q, &mylist.head){
					tmp = list_entry(pos, struct assign_node, head);
					printk("close - list for each safe");
					if((tmp->qid) == msqid){
						list_del(pos);
						kfree(tmp);
					}
					i++;
				}		
				spin_unlock(&my_lock);		
				printk("queue completely close");
			}

			return 0;
		case ASSIGN_IOCTL3://스핀락 처리
			msqid = ((ku_msgsnd_param*)arg)->msqid;
			msgp = ((ku_msgsnd_param*)arg)->msgp;
			msgsz = ((ku_msgsnd_param*)arg)->msgsz;
			msgflg = ((ku_msgsnd_param*)arg)->msgflg;
			
			//assign node돌면서 qid를 찾아 큐의 메시지 개수가 maxmsg초과시 -1 리턴
			spin_lock(&my_lock);
			list_for_each(pos, &mylist.head){
				tmp = list_entry(pos, struct assign_node, head);
				if((tmp->qid) == msqid){
					if(msgflg == KU_IPC_NOWAIT && ((tmp->qList.msgsize >= KUIPC_MAXMSG) || tmp->qList.msgvol >= KUIPC_MAXVOL)){
						printk("nowait인데 큐 다참");
						return -1;
					}else{//nowait이 아니면 lib에서 스핀락걸리게 해야됨
						int ff=0;
						while(ff = isFull(tmp->qid)){
							printk("%d---", ff);
						}

						(tmp->qList.msgsize)++;
						(tmp->qList.msgvol) += msgsz;
						qtmp = (struct queue_node*)kmalloc(sizeof(struct queue_node), GFP_KERNEL);
						cfu = copy_from_user( &qtmp->buf, msgp, msgsz);
			                        list_add_tail(&qtmp->head, &tmp->qList.head);
						printk("type:%ld text:%s textsize:%d", qtmp->buf.type, &(qtmp->buf).text, msgsz );//&qtmp->buf.text는 됨
						printk("msgsize:%d, msgvol:%d", tmp->qList.msgsize, tmp->qList.msgvol);
						return 0;
					}
				}
			}
			spin_unlock(&my_lock);
			return 0;
		case ASSIGN_IOCTL4:
			msqid = ((ku_msgrcv_param*)arg)->msqid;
			msgp = ((ku_msgrcv_param*)arg)->msgp;
			msgsz = ((ku_msgrcv_param*)arg)->msgsz;
			msgtyp = ((ku_msgrcv_param*)arg)->msgtyp;
			msgflg = ((ku_msgrcv_param*)arg)->msgflg;
			
			isExist = false;
			spin_lock(&my_lock);
                        list_for_each(pos, &mylist.head){//큐를 돌면서 큐에메시지가 있는지 확인
                        	tmp = list_entry(pos, struct assign_node, head);
                        	if((tmp->qid) == msqid){
                                 	isExist = true;
					printk("rcv qid find");
					list_for_each(qpos, &tmp->qList.head){
						qtmp = list_entry(qpos, struct queue_node, head);
						if(qtmp->buf.type == msgtyp){
							printk("type:%ld ", (qtmp->buf).type);
							if( msgflg & KU_MSG_NOERROR ){//noerror이면 수신할수있는만큼만
								printk("rcv ioctl : %s", qtmp->buf.text);
								printk("rcv storedsize %d, msgsz:%d", strlen(qtmp->buf.text), msgsz);
								
								ctu = copy_to_user(msgp, &qtmp->buf, msgsz/*+sizeof(long*)*/);
								list_del(qpos);
								kfree(qtmp);
								return msgsz;
							}else{
								return -1;
							}
						}
					}
                                 }
                        }
			spin_unlock(&my_lock);
			if(msgflg & KU_IPC_NOWAIT){//nowait이고 qid가 없으면 -1 즉시 반환
                        	if(isExist == false){
					printk("nowait 즉시반환");
                        		return -1;
				}
			}
			
			break;
		//case ASSIGN_IOCTL5:

		default:
			return -1;
	}

	return 0;
}

static int assign_ioctl_open(struct inode* inode, struct file* file){
	printk("assign_ioctl open");
	return 0;
}

static int assign_ioctl_release(struct inode* inode, struct file* file){
	printk("assign_ioctl release");
	return 0;
}

struct file_operations assign_char_fops = {
	.unlocked_ioctl = assign_ioctl,
	.open = assign_ioctl_open,
	.release = assign_ioctl_release,
};

static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init assign_ioctl_init(void){
	printk("assign init module\n");

	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &assign_char_fops);
	cdev_add(cd_cdev, dev_num, 1);

	INIT_LIST_HEAD(&mylist.head);
	
	return 0;
}

static void __exit assign_ioctl_exit(void){
	printk("assign exit module\n");

	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num,1);
}

module_init(assign_ioctl_init);
module_exit(assign_ioctl_exit);
