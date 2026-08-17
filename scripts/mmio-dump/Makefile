# Out-of-tree build of mmio_dump.ko.
# KDIR must be the build tree of the TARGET kernel — the .ko's vermagic must
# match the kernel on the box you insmod it on.
#   make KDIR=/mnt/2tb/unvr-port-refs/linux-v7.1.8 \
#        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
obj-m := mmio_dump.o

KDIR ?= /lib/modules/$(shell uname -r)/build
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
