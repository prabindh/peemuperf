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
#include <linux/slab.h>

#include "v7_pmu.h"

static void pmu_start(unsigned int event_array[],unsigned int count);
static void pmu_stop(void);
static int __peemuperf_init_checks(void);
static void __exit peemuperf_exit(void);

static int emifcnt = 0;

#if defined(CONFIG_PROC_FS)
#include <linux/proc_fs.h>
#define MAX_PROC_BUF_LEN 1000
static char proc_buf[MAX_PROC_BUF_LEN];
#endif

static int evlist[6] = {1, 0x44, 3, 4, 5, 6};
static int evlist_count = 6;
static int available_evcount = 0;
static int evdelay = 500; //mSec
static int evdebug = 0;

static void pmu_start(unsigned int event_array[],unsigned int count)
{
	int i;
	enable_pmu();              // Enable the PMU
	reset_ccnt();              // Reset the CCNT (cycle counter)
	reset_pmn();               // Reset the configurable counters
	write_flags((1 << 31) | 0xf); //Reset overflow flags
	for(i = 0;i < count;i ++)
	{
		pmn_config(i, event_array[i]);
		if(evdebug == 1) printk("Configured counter[%d] for event 0x%x\n", 
			i, event_array[i]);
	}

	ccnt_divider(1); //Enable divide by 64
	enable_ccnt();  // Enable CCNT

	for(i = 0;i < count;i ++)
		enable_pmn(i);
}

static void pmu_stop(void)
{
	int i;
	unsigned int cycle_count, overflow;
	unsigned int counters = available_evcount;
#if defined(CONFIG_PROC_FS)
	int currProcBufLen = 0;
#endif

	disable_ccnt();

	for(i = 0;i < counters;i ++)
		disable_pmn(i);

	for(i = 0;i < counters;i ++)
	{
		if(evdebug == 1) printk("Counter[%d]= %d\n", i, read_pmn(i));
#if defined(CONFIG_PROC_FS)
		currProcBufLen += sprintf(proc_buf + currProcBufLen,
			"Counter[%d]= %d\n", i, read_pmn(i));
#endif
	}

	cycle_count = read_ccnt(); // Read CCNT
	overflow = read_flags();   //Check for overflow flag

	if(evdebug == 1) printk("Overflow flag: = 0x%x, Cycle Count: = %d \n\n", 
		overflow,cycle_count);
#if defined(CONFIG_PROC_FS)
	currProcBufLen += sprintf(proc_buf + currProcBufLen,
		"Overflow flag: = 0x%x, Cycle Count: = %d \n\n", 
		overflow,cycle_count);
#endif

	if(emifcnt == 1)
	{
		//Now read the READ+WRITE monitoring counters
		 emifreadcount = ioread32(emifcnt_reg_base+0x80);
		 emifwritecount = ioread32(emifcnt_reg_base+0x84);
	}

}


/****************************************************************************
* Monitoring Thread
****************************************************************************/
static int peemuperf_thread(void* data)
{
	if(evdebug == 1) printk("Entering thread loop...\n");
	while(1)
	{
		if(kthread_should_stop()) break;

		pmu_start(evlist, available_evcount);

		msleep(evdelay);

		pmu_stop();

		msleep(10);
	}
	if(evdebug == 1) printk("Exiting thread...\n");
	return 0;
}

#if defined(CONFIG_PROC_FS)
/****************************************************************************
* /proc entries
****************************************************************************/
static int peemuperf_proc_read(char *buf, char **start, off_t offset, 
                          int len, int *unused_i, void *unused_v)
{
	int c, outlen;
	outlen = 0;

	for(c = 0;c < MAX_PROC_BUF_LEN;c ++)
	{
		buf[c] = proc_buf[c];
	}
	return MAX_PROC_BUF_LEN;
}

static __init int register_proc(void)
{
	struct proc_dir_entry *proc_peemuperf;
	int c;

	proc_peemuperf = create_proc_entry("peemuperf", S_IRUGO, NULL);
	if (proc_peemuperf) 
		proc_peemuperf->read_proc = peemuperf_proc_read;

	for(c = 0;c < MAX_PROC_BUF_LEN;c ++)
	{
		proc_buf[c] = '\0';
	}

	return proc_peemuperf != NULL;
}

#endif /* CONFIG_PROC_FS */


/****************************************************************************
* Entry and Exit
****************************************************************************/
static struct task_struct *peemuperf_task;
static struct resource *emifcnt_regs;

static int __peemuperf_init_checks()
{
	if(evdebug == 1) printk("Event inputs: %d %d %d %d\n", evlist[0], 
			evlist[1], evlist[2], evlist[3]);

	available_evcount = getPMN();
	peemuperf_task = kthread_run(peemuperf_thread,(void *)0, 
				"peemuperf_thread");
	
#define EMIFCNT_MAP_LEN 0x100
#define EMIFCNT_MAP_BASE_ADDR 0x4c000000

	if(emifcnt == 1)
	{
		emifcnt_regs = request_mem_region(EMIFCNT_MAP_BASE_ADDR, EMIFCNT_MAP_LEN, "emifcnt");
		if (!emifcnt_regs)
			return 1;
		emifcnt_reg_base = (resource_size_t)ioremap_nocache(emifcnt_regs->start, 8);
		if (!emifcnt_reg_base)
			return 1;
		//Now enable READ+WRITE monitoring counters
		 iowrite32(0x80028003, emifcnt_reg_base+0x88);
	}

	return 0;
}

static int __init peemuperf_init(void)
{
	unsigned int retVal = 0;

	retVal = __peemuperf_init_checks();
	if(retVal)
	{
		if(evdebug == 1) printk("%s: peemuperf checks failed\n", __FUNCTION__);
		return -ENODEV;
	}
	else
	{
		if(evdebug == 1) printk("peemuperf checks passed...\n");
	}
#if defined(CONFIG_PROC_FS)
	register_proc();
#endif
	return 0;
}

static void __exit peemuperf_exit()
{
	kthread_stop(peemuperf_task);
#if defined(CONFIG_PROC_FS)
	remove_proc_entry("peemuperf", NULL);
#endif

	if(emifcnt == 1)
	{
		release_mem_region(EMIFCNT_MAP_BASE_ADDR, EMIFCNT_MAP_LEN);
		iounmap(emifcnt_reg_base);
	}
}


/****************************************************************************
* Configuration
****************************************************************************/
module_param(evdelay, int, 500);
module_param(evdebug, int, 0);
module_param_array(evlist, int, &evlist_count, 0000);
module_param(emifcnt, int, 0);

late_initcall(peemuperf_init);
module_exit(peemuperf_exit);
MODULE_DESCRIPTION("PMU driver - insmod peemuperf.ko evdelay=500 evlist=1,68,3,4 evdebug=1");
MODULE_AUTHOR("Prabindh Sundareson <prabu@ti.com>");
MODULE_LICENSE("GPL v2");

