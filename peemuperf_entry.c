/* 
 * peemuperf - entry file
 *
 * Prabindh Sundareson (prabu@ti.com) 2012
 * */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#include "v7_pmu.h"

static void pmu_start(unsigned int event0,unsigned int event1,unsigned int event2,
				unsigned int event3,unsigned int event4,unsigned int event5);
static void pmu_stop(void);
static int __peemuperf_init_checks(void);
static void __exit peemuperf_exit(void);


static void pmu_start(unsigned int event0,unsigned int event1,unsigned int event2,
			unsigned int event3,unsigned int event4,unsigned int event5)
{
	enable_pmu();              // Enable the PMU
	reset_ccnt();              // Reset the CCNT (cycle counter)
	reset_pmn();               // Reset the configurable counters
	pmn_config(0, event0);       // Configure counter 0 to count event code 0x03
	pmn_config(1, event1);       // Configure counter 1 to count event code 0x03
	pmn_config(2, event2);       // Configure counter 2 to count event code 0x03
	pmn_config(3, event3);       // Configure counter 3 to count event code 0x03
	pmn_config(4, event4);       // Configure counter 4 to count event code 0x03
	pmn_config(5, event5);       // Configure counter 5 to count event code 0x03

	enable_ccnt();             // Enable CCNT
	enable_pmn(0);             // Enable counter
	enable_pmn(1);             // Enable counter
	enable_pmn(2);             // Enable counter
	enable_pmn(3);             // Enable counter
	enable_pmn(4);             // Enable counter
	enable_pmn(5);             // Enable counter

	printk("CountEvent0=0x%x,CountEvent1=0x%x,CountEvent2=0x%x, \
			CountEvent3=0x%x, \
		CountEvent4=0x%x,CountEvent5=0x%x\n",
			event0,event1,event2,event3,event4,event5);
}

static void pmu_stop(void)
{
	unsigned int cycle_count, overflow, counter0, counter1, counter2, 
		counter3, counter4, counter5;

	disable_ccnt();            // Stop CCNT
	disable_pmn(0);            // Stop counter 0
	disable_pmn(1);            // Stop counter 1
	disable_pmn(2);            // Stop counter 2
	disable_pmn(3);            // Stop counter 3
	disable_pmn(4);            // Stop counter 4
	disable_pmn(5);            // Stop counter 5

	counter0    = read_pmn(0); // Read counter 0
	counter1    = read_pmn(1); // Read counter 1
	counter2    = read_pmn(2); // Read counter 2
	counter3    = read_pmn(3); // Read counter 3
	counter4    = read_pmn(4); // Read counter 4
	counter5    = read_pmn(5); // Read counter 5

	cycle_count = read_ccnt(); // Read CCNT
	overflow=read_flags();        //Check for overflow flag

	printk("Counter0=%d,Counter1=%d,Counter2=%d,Counter3=%d,Counter4=%d, \
		Counter5=%d\n", counter0, counter1,counter2,counter3,
			counter4,counter5);
	printk("Overflow flag: = %d, Cycle Count: = %d \n\n", 
		overflow,cycle_count);
}


/****************************************************************************
* Monitoring Thread
****************************************************************************/
static int peemuperf_thread(void* data)
{
	printk("Entering thread loop...\n");
	while(1)
	{
		if(kthread_should_stop()) break;

		pmu_start(0x01,0x02,0x03,0x04,0x05,0x06);
		mdelay(500);
		pmu_stop();
	}
	printk("Exiting thread...\n");
	return 0;
}

/****************************************************************************
* Entry and Exit
****************************************************************************/
static struct task_struct *peemuperf_task;
static int __peemuperf_init_checks()
{
	peemuperf_task = kthread_run(peemuperf_thread,(void *)0, 
				"peemuperf_thread");
	return 0;
}

static int __init peemuperf_init(void)
{
	unsigned int retVal = 0;

	retVal = __peemuperf_init_checks();
	if(retVal)
	{
		printk("%s: peemuperf checks failed\n", __FUNCTION__);
		return -ENODEV;
	}
	else
	{
		printk("peemuperf checks passed...\n");
	}
	return 0;
}

static void __exit peemuperf_exit()
{
	kthread_stop(peemuperf_task);
}

/****************************************************************************
* Configuration
****************************************************************************/
late_initcall(peemuperf_init);
module_exit(peemuperf_exit);
MODULE_DESCRIPTION("PEEMUPERF - ARM PMU based monitoring");
MODULE_AUTHOR("Prabindh Sundareson <prabu@ti.com>");
MODULE_LICENSE("GPL v2");

