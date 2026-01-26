obj-m += ina219.o

PWD := $(shell pwd)

.PHONY: all clean install build

all: build install

build:
	@$(MAKE) -C $(KERNEL_BUILD_PATH) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) \
		modules


install:
	@$(MAKE) -C $(KERNEL_BUILD_PATH) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) \
		modules_install


clean:
	$(MAKE) -C $(KERNEL_BUILD_PATH) \
		M=$(PWD) \
		clean

