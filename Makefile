-include Rules.make

TARGET = peemuperf.ko
obj-m = peemuperf.o

peemuperf-objs = peemuperf_entry.o v7_pmu.o

MAKE_ENV = ARCH=arm CROSS_COMPILE=$(TOOLCHAIN_PATH)

.PHONY: release

default: release

release:
	make -C $(LINUXKERNEL_INSTALL_DIR) M=`pwd` $(MAKE_ENV) modules



