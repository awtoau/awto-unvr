# Makefile for Annapurna Labs al_eth out-of-tree kernel module
# Target: Linux 6.12 LTS
#
# Usage:
#   make KDIR=/path/to/kernel/build
#   make KDIR=/path/to/kernel/build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
#
# For the Alpine V2 SoC (aarch64):
#   make KDIR=/path/to/linux-6.12-build ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

# Kernel build directory (override with KDIR= on command line)
KDIR ?= /lib/modules/$(shell uname -r)/build

# MODULE_VARIANT selects which .ko this Makefile produces from the same
# sources (#131 - one module bound to both the 1G and 10G PCI devices let
# udev's two independent modprobes race on one finit_module()):
#   (unset)  al_eth.ko      - original combined table (both PCI IDs)
#   1g       al_eth_1g.ko   - PCI_DEVICE_ID_AL_ETH only
#   10g      al_eth_10g.ko  - PCI_DEVICE_ID_AL_ETH_ADVANCED only
# Build each variant in its own $(CURDIR) copy - Kbuild tracks per-object
# cmdline flags in .*.o.cmd, but re-running here with a different variant
# would still serialize a same-directory rebuild, so callers use separate
# source copies (see scripts/build-linux-fedora.py).
MODULE_VARIANT ?=

ifeq ($(MODULE_VARIANT),1g)
MODNAME := al_eth_1g
ccflags-y += -DAL_ETH_BUILD_1G_ONLY=1
else ifeq ($(MODULE_VARIANT),10g)
MODNAME := al_eth_10g
ccflags-y += -DAL_ETH_BUILD_10G_ONLY=1
else
MODNAME := al_eth
endif

obj-m := $(MODNAME).o

# Object files that make up the module
$(MODNAME)-y := al_eth_main.o \
            al_eth_phylink.o \
            al_eth_sysfs.o \
            al_hal_eth_kr.o \
            al_hal_eth_main.o \
            al_hal_iofic.o \
            al_hal_serdes_25g.o \
            al_hal_serdes_hssp.o \
            al_hal_udma_config.o \
            al_hal_udma_debug.o \
            al_hal_udma_iofic.o \
            al_hal_udma_main.o \
            al_init_eth_kr.o \
            al_init_eth_lm.o \
            al_serdes.o \
            alpine_serdes.o \
            kcompat.o

# Extra CFLAGS for the module
ccflags-y += -I$(src)

.PHONY: all clean modules_install

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

modules_install:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules_install
