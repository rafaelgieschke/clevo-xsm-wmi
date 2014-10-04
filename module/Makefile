obj-m += clevo-xsm-wmi.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
#CFLAGS_clevo-xsm-wmi.o := -DDEBUG

all:
	make -C $(KDIR) M=$(PWD) modules

install:
	make -C $(KDIR) M=$(PWD) modules_install

clean:
	make -C $(KDIR) M=$(PWD) clean
