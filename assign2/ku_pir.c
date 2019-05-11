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
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/rculist.h>
#include "ku_pir.h"

MODULE_LICENSE("GPL");

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

spinlock_t my_lock;
static int irq_num;
wait_queue_head_t my_wq;

struct queue_node{
	struct list_head head;
	struct ku_pir_data data;
	//int queue_size;
};

struct assign_node{
	struct list_head head;
	struct queue_node qlist;
	int fd;//변수명이 fd이긴한데 pid를 저장함
	int queue_size;
};

struct assign_node mylist;
struct assign_node *tmp = 0;
struct queue_node *qtmp = 0;
struct list_head *q = 0;
struct list_head *pos = 0;
struct list_head *qpos = 0;

static int get_queue_size(int pid){
	list_for_each_entry(tmp, &mylist.head, head){
		if((tmp->fd) == pid){
			return tmp->queue_size;
			
		}
	}
	return -1;
}

static long assign_ioctl(struct file* file, unsigned int cmd, unsigned long arg){

	int tmp_pid = 0;

	int ku_pir_open_fd = 0;

	int ku_pir_close_fd = 0;

	int ku_pir_read_fd = 0;
	struct ku_pir_data *ku_pir_read_data;

	int ku_pir_flush_fd = 0;

	struct ku_pir_data ku_pir_insertData_data;

	switch(cmd){
		case ASSIGN_IOCTL1://ku_pir_open()
			ku_pir_open_fd = (int)arg;
			//printk("ku_pir_open_fd:%d\n", ku_pir_open_fd);
			printk("ioctl_open_pid:%d\n", current->pid);
			tmp = (struct assign_node*)kmalloc(sizeof(struct assign_node), GFP_KERNEL);
			
			INIT_LIST_HEAD(&tmp->qlist.head);
			tmp->fd = current->pid;//ku_pir_open_fd;
			tmp->queue_size = 0;//큐사이즈
		
			list_add_tail/*_rcu*/(&tmp->head, &mylist.head);	
			printk("ioctl_list open!!!!!\n");
			
			break;
		case ASSIGN_IOCTL2://ku_pir_close(int fd)
			ku_pir_close_fd = (int)arg;
			//printk("ioctl_ku_pir_close_fd:%d\n", ku_pir_close_fd);
			//rcu_read_lock();
			printk("ioctl_close_pid:%d\n", current->pid);			
			list_for_each_safe(pos, q, &mylist.head){
				tmp = list_entry(pos, struct assign_node, head);
				//printk("ioctl_ tmp->fd:%d   ku_pir_close_fd:%d\n", tmp->fd, ku_pir_close_fd);
				if((tmp->fd) == current->pid){
					list_del/*_rcu*/(pos);
					kfree(tmp);
					printk("delete complete\n");
					return 0;
				}
			
			}		
			//rcu_read_unlock();

			return -1;//해당 fd의 자료구조가 없으면 -1을 반환
		case ASSIGN_IOCTL3://ku_pir_read(int fd,struct ku_pir_data *data)
			//int tmp_size;
			ku_pir_read_fd = ((struct ku_pir_read_param*)arg)->fd;
			ku_pir_read_data = ((struct ku_pir_read_param*)arg)->data;
			//spin_lock(&my_lock);
			list_for_each_entry(tmp, &mylist.head, head){
				if( ((tmp->fd) == current->pid) && ((tmp->fd) != 0) ){
					//printk("-----%d\n",tmp->fd);
					//wait_event_interruptible(my_wq, (tmp->queue_size)>0);
					//printk("wait pid:%d end wait size: %d\n", tmp->fd, tmp->queue_size);
					tmp_pid = tmp->fd;
					break;
				}
			}
			//printk("wait tmpsize :%d\n", tmp_size);
			wait_event_interruptible(my_wq, get_queue_size(tmp_pid)>0);
			//printk("wait tempsize :%d\n", );

			//rcu_read_lock();
			spin_lock(&my_lock);
			list_for_each_entry/*_rcu*/(tmp, &mylist.head, head){//
				if((tmp->fd) == current->pid){
					printk("wait2 pid:%d end2 wait size:%d\n", tmp->fd, tmp->queue_size);
					list_for_each_safe(qpos, q, &tmp->qlist.head){
						qtmp = list_entry(qpos, struct queue_node, head);
						copy_to_user(ku_pir_read_data, &qtmp->data, sizeof(struct ku_pir_data));
						printk("ioctl_read_pid:%d\n", current->pid);
						printk("&qtmp->data.data: %ld, %c\n", (qtmp->data).timestamp, (qtmp->data).rf_flag);
						list_del/*_rcu*/(qpos);//
						kfree(qtmp);
						(tmp->queue_size)--;

						return 0;
					}
				}
			}
			spin_unlock(&my_lock);
			//rcu_read_unlock();

			return 0;
		case ASSIGN_IOCTL4://ku_pir_flush(int fd)
			ku_pir_flush_fd = (int)arg;
			
			//rcu_read_lock();
                        list_for_each_entry/*_rcu*/(tmp, &mylist.head, head){//
                                if((tmp->fd) == current->pid){
					list_for_each_safe(qpos, q, &(tmp->qlist).head){
						qtmp = list_entry(qpos, struct queue_node, head);
						list_del/*_rcu*/(qpos);//
						kfree(qtmp);
						printk("ioctl_flushing...\n");
					}
					(tmp->queue_size) = 0;
                                }
                             
                        }
			//rcu_read_unlock();

			break;
		case ASSIGN_IOCTL5://ku_pir_insertdata(long unsigned int ts, char rf_flag)
			ku_pir_insertData_data = ((struct ku_pir_insertData_param*)arg)->data;
			printk("ioctl_insertData %ld, %d\n", ku_pir_insertData_data.timestamp, ku_pir_insertData_data.rf_flag);	
			
			//rcu_read_lock();
			spin_lock(&my_lock);
			list_for_each_entry/*_rcu*/(tmp, &mylist.head, head){//
				qtmp = (struct queue_node*)kmalloc(sizeof(struct queue_node), GFP_KERNEL);
				qtmp->data.timestamp = ku_pir_insertData_data.timestamp;
				qtmp->data.rf_flag = ku_pir_insertData_data.rf_flag;

                		list_add_tail/*_rcu*/(&qtmp->head, &tmp->qlist.head);//
				printk("ioctl_insertData - list add\n");

				(tmp->queue_size)++;
				printk("insertData pid:%d size: %d\n", tmp->fd, tmp->queue_size);
				//wake_up(&my_wq);
			}

			list_for_each_entry/*_rcu*/(tmp, &mylist.head, head){//
		                if( (tmp->queue_size) > KUPIR_MAX_MSG ){

        		                list_for_each_safe(qpos, q, &(tmp->qlist).head){
        	        	                qtmp = list_entry(qpos, struct queue_node, head);

        	                	        list_del/*_rcu*/(qpos);//
        	                        	kfree(qtmp);
        	                        	(tmp->queue_size)--;
	
                                		break;
                       			}

                		}

        		}
			spin_unlock(&my_lock);
			//rcu_read_unlock();
			
			wake_up(&my_wq);

			break;
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

static irqreturn_t kupir_sensor_isr(int irq, void* dev_id){
        printk("kupir detect\n");
	
	list_for_each_entry_rcu(tmp, &mylist.head, head){
		
		qtmp = (struct queue_node*)kmalloc(sizeof(struct queue_node), GFP_KERNEL);
		qtmp->data.timestamp = jiffies;
	        if(gpio_get_value(KUPIR_SENSOR) == 1){//rising
	                qtmp->data.rf_flag = '0';
	        }else{//falling
	                qtmp->data.rf_flag = '1';
	        }

		list_add_tail_rcu(&qtmp->head, &tmp->qlist.head);
		printk("detect - list add ");
		
		(tmp->queue_size)++;

	}
	
	list_for_each_entry_rcu(tmp, &mylist.head, head){
		if(tmp->queue_size > KUPIR_MAX_MSG){
			
			list_for_each_safe(qpos, q, &(tmp->qlist).head){
				qtmp = list_entry(qpos, struct queue_node, head);
				
				list_del_rcu(qpos);                                                
				kfree(qtmp);                                
				(tmp->queue_size)--;

				break;
			}

		}

	}

	wake_up_interruptible(&my_wq);

        return IRQ_HANDLED;
}


static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init assign_ioctl_init(void){
	int ret;
	printk("assign init module\n");

	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &assign_char_fops);
	cdev_add(cd_cdev, dev_num, 1);
	
	spin_lock_init(&my_lock);
	INIT_LIST_HEAD(&mylist.head);
	init_waitqueue_head(&my_wq);

	gpio_request_one(KUPIR_SENSOR, GPIOF_IN, "kupir_sensor");
	irq_num = gpio_to_irq(KUPIR_SENSOR);
	ret = request_irq(irq_num, kupir_sensor_isr, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "sensor_irq", NULL);
	if(ret){
		printk(KERN_ERR "Unable to request IRQ:%d\n", ret);
		free_irq(irq_num,NULL);
	}
	return ret;
}

static void __exit assign_ioctl_exit(void){
	printk("assign exit module\n");

	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num,1);
	
	disable_irq(irq_num);
	free_irq(irq_num, NULL);
	gpio_free(KUPIR_SENSOR);

}

module_init(assign_ioctl_init);
module_exit(assign_ioctl_exit);
