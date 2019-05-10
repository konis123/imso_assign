#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/unistd.h>
#include <linux/delay.h>

#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

MODULE_LICENSE("GPL");

#define PIR 17
#define LED1 16
#define LED2 20
#define LED3 21	//PIR 입력이 오면 반응하는 LED

struct task_struct *test_task1 = NULL;
struct task_struct *test_task2 = NULL;
static struct workqueue_struct *test_wq;

typedef struct{
	struct work_struct my_work;
	//struct my_data data;
}my_work_t;

my_work_t *work;

static int irq_num1;
static int irq_num2;
static int irq_num3;
static int irq_num4;

static void my_wq_func(struct work_struct *work){
	/*
	my_work_t *my_work = (my_work_t*)work;
	(my_work->data).year = 2019;
	(my_work->data).month = 5;
	(my_work->data).day = 10;
	printk("today is %d/%d/%d\n", (my_work->data).year, (my_work->data).month, (my_work->data).day);
	kfree((void*)work);
	*/
	printk("LED3\n");
	my_work_t *my_work = (my_work_t*)work;
	if(gpio_get_value(LED3) == 0){
		gpio_set_value(LED3, 1);
	}else{
		gpio_set_value(LED3, 0);
	}
	kfree((void*)work);
}

static irqreturn_t simple_pir_isr(int irq, void* data){
	int ret;
	printk("ISR start \n");

	if(test_wq){
		work = (my_work_t*)kmalloc(sizeof(my_work_t), GFP_KERNEL);
		if(work){
			INIT_WORK((struct work_struct*)work, my_wq_func);
			ret = queue_work(test_wq, (struct work_struct*)work);
		}
	}
	return IRQ_HANDLED;
}

int thread_func1(void *data){
	while(!kthread_should_stop()){
		//printk("LED 1	jiffies in thread : %ld\n", jiffies);
		
		if(gpio_get_value(LED1) == 0){
	                gpio_set_value(LED1, 1);
			printk("LED1 on");
        	}else{
			gpio_set_value(LED1, 0);
			printk("LED1 off");
		}

		msleep(1000);
	}
	return 0;
}

int thread_func2(void *data){
	while(!kthread_should_stop()){
		//printk("LED 1.5   jiffies in thread : %ld\n", jiffies);

		if(gpio_get_value(LED2) == 0){
	                gpio_set_value(LED2, 1);
			printk("LED1.5 on");
	        }else{
	                gpio_set_value(LED2, 0);
			printk("LED1.5 off");
	        }

		msleep(1500);
	}
	return 0;
}

static int __init kernthread_init(void){
	int ret;

	printk("Init Module\n");

	test_wq = create_workqueue("test_workqueue");

	gpio_request_one(PIR, GPIOF_IN, "PIR");
	gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");
	gpio_request_one(LED2, GPIOF_OUT_INIT_LOW, "LED2");
	gpio_request_one(LED3, GPIOF_OUT_INIT_LOW, "LED3");
	irq_num1 = gpio_to_irq(PIR);
	irq_num2 = gpio_to_irq(LED1);
	irq_num3 = gpio_to_irq(LED2);
	irq_num4 = gpio_to_irq(LED3);
	ret = request_irq(irq_num1, simple_pir_isr, IRQF_TRIGGER_RISING, "pir_irq", NULL);
	if(ret){
		printk(KERN_ERR "Unable to request IRQ:%d\n", ret);
		free_irq(irq_num1, NULL);
	}
	test_task1 = kthread_create(thread_func1, NULL, "my_thread1");
	test_task2 = kthread_create(thread_func2, NULL, "my_thread2");
	if(IS_ERR(test_task1)){
		test_task1 = NULL;
		printk("test1 kernel thread ERROR\n");
	}

	if(IS_ERR(test_task2)){
		test_task2 = NULL;
		printk("test2 kernel thread ERROR\n");
	}

	wake_up_process(test_task1);
	wake_up_process(test_task2);

	return 0;
}

static void __exit kernthread_exit(void){
	printk("Exit Module\n");

	free_irq(irq_num1, NULL);
	free_irq(irq_num2, NULL);
	free_irq(irq_num3, NULL);
	free_irq(irq_num4, NULL);
	gpio_free(PIR);
	gpio_free(LED1);
	gpio_free(LED2);
	gpio_free(LED3);
	if(test_task1){
		kthread_stop(test_task1);
		printk("test1 kernel thread STOP");
	}
	if(test_task2){
		kthread_stop(test_task2);
		printk("test2 kernel thread STOP");
	}
	flush_workqueue(test_wq);
	destroy_workqueue(test_wq);
}

module_init(kernthread_init);
module_exit(kernthread_exit);
