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

struct evlist_struct
{
	struct list_head list;
	u32 event_id;
};
LIST_HEAD(evlist_head);

//static char* evlist;
static void parse_evlist(char* evlist)
{
	char* curr = strsep(&evlist, "0x");
	int ret;
	int count = 0;
	while(curr)
	{
		struct evlist_struct *curr_evlist = 
			kmalloc(sizeof(struct evlist_struct), GFP_KERNEL);
		INIT_LIST_HEAD(&curr_evlist->list);
		ret = kstrtou32(curr, 0, &curr_evlist->event_id);

		if(evdebug == 1) printk("string = %s, Input curr_evlist->event_id = %d\n", 
			curr, curr_evlist->event_id);
		list_add(&curr_evlist->list, &evlist_head);

		curr = strsep(&curr, "0x");
		if(count++ > 6)
		{
			if(evdebug == 1) printk("More than expected inputs (%d)\n", count);
			break;
		}
	}
}

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

		//pmu_start(0x01,0x02,0x03,0x04,0x05,0x06);
		//1 - IFETCH MISS, 0x44 - L2 CACHE MISS, 
		//pmu_start(0x01,0x44,0x03,0x04);
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
static int __peemuperf_init_checks()
{
	if(evdebug == 1) printk("Event inputs: %d %d %d %d\n", evlist[0], 
			evlist[1], evlist[2], evlist[3]);

	available_evcount = getPMN();
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
}


/****************************************************************************
* Configuration
****************************************************************************/
//module_param_named(omapes, sgxdbgkm_omapes, int, 5);
module_param(evdelay, int, 500);
module_param(evdebug, int, 0);
module_param_array(evlist, int, &evlist_count, 0000);

late_initcall(peemuperf_init);
module_exit(peemuperf_exit);
MODULE_DESCRIPTION("PMU driver - insmod peemuperf.ko evdelay=500 evlist=1,68,3,4 evdebug=1");
MODULE_AUTHOR("Prabindh Sundareson <prabu@ti.com>");
MODULE_LICENSE("GPL v2");

