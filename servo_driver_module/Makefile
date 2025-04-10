obj-m := servo_driver_module.o
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
INCLUDE_DIR := $(PWD)/include

EXTRA_CFLAGS := -I$(INCLUDE_DIR)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" modules
	$(info EXTRA_CFLAGS is: $(EXTRA_CFLAGS))

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean