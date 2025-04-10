obj-m := servo_driver_module.o
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
INCLUDE_DIR := $(PWD)/include

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) EXTRA_CFLAGS="-I$(INCLUDE_DIR)" modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean