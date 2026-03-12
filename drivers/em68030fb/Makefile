obj-m := em68030fb.o
KDIR  := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

install:
	install -m 644 em68030fb.ko /lib/modules/$(shell uname -r)/extra/
	depmod -a

.PHONY: all clean install
