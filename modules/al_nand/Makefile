# Makefile for the Annapurna Labs al_nand out-of-tree kernel module
#
# Usage:
#   make KDIR=/path/to/kernel/build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

# Kernel build directory (override with KDIR= on command line)
KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m := al_nand.o

# Driver glue + the NAND HAL. al_hal_nand_dma.c is NOT built: the vendor's own
# Makefile never built it either (DMA read/write is an unfinished vendor TODO),
# and leaving it out keeps the al_hal_ssm_raid dependency out of the module.
al_nand-y := al_nand_main.o \
             al_hal_nand.o

ccflags-y += -I$(src)

.PHONY: all clean modules_install

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

modules_install:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules_install
