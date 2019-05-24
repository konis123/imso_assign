#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

#define PIN1 6
#define PIN2 13
#define PIN3 19
#define PIN4 26

#define STEPS 8

int steps[STEPS][4] = {
	{1,0,0,0},
	{1,1,0,0},
	{0,1,0,0},
	{0,1,1,0},
	{0,0,1,0},
	{0,0,1,1},
	{0,0,0,1},
	{1,0,0,1}
};

void setStep(int step){
	gpio_set_value(PIN1, steps[step][0]);
    gpio_set_value(PIN2, steps[step][1]);
    gpio_set_value(PIN3, steps[step][2]);
    gpio_set_value(PIN4, steps[step][3]);
}

void forward(int round, int delay){
	int i=0;
	int j=0;
	for(i=1;i<=64*round;i++){
		for(j=1;j<=64;j++){//5.625도
			setStep(j%8);
			udelay(delay);
		}
	}

}

void moveDegree(int degree, int delay, int direction){
	int i=0;
	if(direction == 0){
		for(i=1;i<=(64*64*degree)/360;i++){//360도
			setStep(i%8);
			udelay(delay);
		}

	}else{
		for(i=1;i<=(64*64*degree)/360;i++){
			setStep(STEPS-i%8);
			udelay(delay);
		}	
	}



}

static int __init simple_motor_init(void){
	gpio_request_one(PIN1, GPIOF_OUT_INIT_LOW, "p1");
	gpio_request_one(PIN2, GPIOF_OUT_INIT_LOW, "p2");
	gpio_request_one(PIN3, GPIOF_OUT_INIT_LOW, "p3");
	gpio_request_one(PIN4, GPIOF_OUT_INIT_LOW, "p4");

	//forward(1, 1200);
	
	moveDegree(45, 1000, 0);
	mdelay(1000);
	moveDegree(90, 1000, 1);
	mdelay(1000);
	moveDegree(180, 1000, 0);
	
	return 0;
}

static void __exit simple_motor_exit(void){
	gpio_free(PIN1);
	gpio_free(PIN2);
	gpio_free(PIN3);
	gpio_free(PIN4);
}

module_init(simple_motor_init);
module_exit(simple_motor_exit);
