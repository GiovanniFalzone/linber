MODULE_MAME = linber

SRCS = $(MODULE_MAME)_main.c src/linber_RBtree.c
OBJS = $(SRCS:.c=.o)

obj-m += $(MODULE_MAME).o
$(MODULE_MAME)-y = $(OBJS)

INCLUDE_DIR = -I$(src)/inc -I$(src)/../libs/inc
ccflags-y := $(INCLUDE_DIR)


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *~ *.bak

