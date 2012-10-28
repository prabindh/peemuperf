peemuperf
=========

Linux Performance Monitoring using PMU - ARM Cycles, Cache misses, and more ...

Usage
=========
--> insmod peemuperf.ko evdelay=500 evlist=1,68,3,4 evdebug=1 emifcnt=1

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

emifcnt = 0 (default - no usage of EMIF DDR monitor for READ/write count) / 1 (use EMIF DDR monitor). For more details, refer to EMIF documentation for example at www.ti.com/lit/ug/sprugv8c/sprugv8c.pdf

Output
=======
An example console output when evdebug = 1, is below (CPU MHz = 720 MHz, sampling every 1000 millisec):
(EMIF outputs are not shown - truncated)

[  100.066070] 49031    2388256 173679  20677707        0       11362862       
[  101.096008] 50458    2322475 169335  20240942        0       11362603       
[  102.126007] 75056    2384952 184637  20614303        0       11362506       
[  103.156036] 49902    2322703 169203  20224376        0       11362827       
[  104.186035] 48974    2320928 168278  20198781        0       11362768       


The Counter values, Overflow indication, Cycle count, EMIF read and write count is provided.
NOTE: PMU Cycle count is configured to be divided by 64, compared to CPU clock

Usage of proc entry
===================
peemuperf exposes event information (same information as above) via proc entry, and can be used by userland applications

cat /proc/peemuperf

root@am335x-evm:/media/sda1# cat /proc/peemuperf  
PMU.counter[0]= 54363                                                           
PMU.counter[1]= 2339356                                                         
PMU.counter[2]= 172534                                                          
PMU.counter[3]= 20313519                                                        
PMU.overflow= 0                                                                 
PMU.CCNT= 11362787                                                              
EMIF.readcount= 1705266985                                                      
EMIF.writecount= 2316069476                                                   

Validation
=========
Tested on kernel 3.2, with gcc4.5 toolchain
Validated on AM335x (OMAP3/Beagle variants will work) - for Cortex-A8. Cortex-A9 has larger counter list, and will also work

Limitations
===========
Interrupts not enabled and not supported, so watchout for overflow flags manually

===
prabindh@yahoo.com
