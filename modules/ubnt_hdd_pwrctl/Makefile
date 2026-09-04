obj-m += ubnt_hdd_pwrctl.o

# Build against the patched tree we ship (override with KDIR=...).
KDIR ?= /mnt/2tb/unvr-port-refs/linux-v7.1.8
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
