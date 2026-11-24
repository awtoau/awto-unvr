obj-m += ubnt_common.o ubnthal.o ui_hdd_pwrctl.o

# ui_hdd_pwrctl needs ubnt_hdd_pwrctl's exports: pass its Module.symvers via
# KBUILD_EXTRA_SYMBOLS (scripts/_kbuild.py does).
KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
