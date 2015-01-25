TARGET = hook
hook-objs := hook_example.o
obj-m += $(TARGET).o

.PHONY: all clean

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
