obj-m += clevo-xsm-wmi.o
KERNELRELEASE ?= $(shell uname -r)
KDIR := /lib/modules/$(KERNELRELEASE)/build
PWD := $(shell pwd)
#CFLAGS_clevo-xsm-wmi.o := -DDEBUG
CFLAGS_clevo-xsm-wmi.o := -DEXPERIMENTAL

all:
	make -C $(KDIR) M=$(PWD) modules

install:
	make -C $(KDIR) M=$(PWD) modules_install

clean:
	make -C $(KDIR) M=$(PWD) clean
