#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

#define SENSOR1 18
#define LED1 17

#define DEV_NAME "c4_dev"

static int irq_num;
static struct timer_list my_timer;

static int c4_open(struct inode* inode, struct file* file){
	printk("c4 open\n");
	enable_irq(irq_num);
	return 0;
}

static int c4_release(struct inode *inode, struct file* file){
	printk("c4 close \n");
	disable_irq(irq_num);
	return 0;
}

struct file_operations c4_fops =
{
	.open = c4_open,
	.release = c4_release,
};

static void c4_func(unsigned long data){
	gpio_set_value(LED1, 0);
}

static irqreturn_t c4_isr(int irq, void* dev_id){
	int v;
	printk("c4 detect \n");
	del_timer(&my_timer);
	v = gpio_get_value(LED1);
	if(v == 0){
		gpio_set_value(LED1, 1);
	        init_timer(&my_timer);
	        my_timer.function = c4_func;
	        //my_timer.data = 1L;
       		my_timer.expires = jiffies + (2*HZ);
	        add_timer(&my_timer);
	}else{
		gpio_set_value(LED1, 0);
		gpio_set_value(LED1, 1);
		init_timer(&my_timer);
		my_timer.function = c4_func;
		my_timer.expires = jiffies + (2*HZ);
		add_timer(&my_timer);
	}
	
	return IRQ_HANDLED;
}

static dev_t dev_num;
static struct cdev *cd_cdev;

static int __init c4_init(void){
	int ret;

	printk("c4 Init Module\n");
	gpio_set_value(LED1, 1);

	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &c4_fops);
	cdev_add(cd_cdev, dev_num, 1);

        gpio_request_one(LED1, GPIOF_OUT_INIT_LOW, "LED1");

	gpio_request_one(SENSOR1, GPIOF_IN, "sensor1");
	irq_num = gpio_to_irq(SENSOR1);
	ret = request_irq(irq_num, c4_isr, IRQF_TRIGGER_FALLING, "c4_irq", NULL);
	if(ret){
		printk(KERN_ERR "Unable to request IRQ:%d\n", ret);
		free_irq(irq_num,NULL);
	}
	else{
		disable_irq(irq_num);
	}

	return 0;
}

static void __exit c4_exit(void){
	printk("Exit Module\n");

	gpio_set_value(LED1, 0);
	gpio_free(LED1);
	
	del_timer(&my_timer);

	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);

	free_irq(irq_num, NULL);
	gpio_free(SENSOR1);
}

module_init(c4_init);
module_exit(c4_exit);
