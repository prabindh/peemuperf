peemuperf
=========

Linux Performance Monitoring using PMU - ARM Cycles, Cache misses, and more ...

Usage
=========
--> insmod peemuperf.ko evdelay=500 evlist=1,68,3,4 evdebug=1

--> rmmod peemuperf.ko

Parameters:

evdelay = Delay between successive reading of samples from event counters (milliSec)

evlist = Decimal value of event IDs to be monitored (refer ARM TRM). If not specified, below are used:

   1 ==> Instruction fetch that causes a refill at the lowest level of instruction or unified cache

   68 ==> Any cacheable miss in the L2 cache

   3 ==> Data read or write operation that causes a refill at the lowest level of data or unified cache

   4 ==> Data read or write operation that causes a cache access at the lowest level of data or unified cache

(for other valid values, refer to Cortex-A TRM)

evdebug = 0 (default - no messages from kernel module) / 1 (event counters are printed out - warning: will flood the console)

Output
=======
An example console output when evdebug = 1, is below (CPU MHz = 720 MHz, sampling every 500 millisec):

[  137.040557] CountEvent0=0x1,CountEvent1=0x44,CountEvent2=0x3,       
       
[  137.556121] Counter0=233636,Counter1=930969,Counter2=772476,Counter3=41281941

[  137.563568] Overflow flag: = 0, Cycle Count: = 5799665

The Event IDs, the Counter values, and Overflow indication, along with Cycle count is provided.
NOTE: Cycle count is configured to be divided by 64, compared to CPU clock

Usage of proc entry
===================
peemuperf exposes event information (same information as above) via proc entry, and can be used by userland applications

cat /proc/peemuperf

Validation
=========
Tested on kernel 3.2, with gcc4.5 toolchain
Validated on AM335x (OMAP3/Beagle variants will work) - for Cortex-A8. Cortex-A9 has larger counter list, and will also work

Limitations
===========
Interrupts not enabled and not supported, so watchout for overflow flags manually

===
prabindh@yahoo.com
