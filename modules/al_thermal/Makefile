# Makefile for Annapurna Labs thermal sensor out-of-tree kernel module
#
# Usage:
#   make KDIR=/path/to/kernel/build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m := al_thermal.o
al_thermal-objs := al_thermal_main.o al_hal_thermal_sensor.o al_hal_pbs_utils.o

ccflags-y += -I$(src)

.PHONY: all clean modules_install

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

modules_install:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules_install
