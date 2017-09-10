obj-m   := proc_test_1.o
KERNEL  := /lib/modules/`uname -r`/build #for mint/ubuntu  
  
all:  
	make -C $(KERNEL) M=`pwd` modules  
  
install:  
	make -C $(KERNEL) M=`pwd` modules_install  
	depmod -A  
  
clean:  
	make -C $(KERNEL) M=`pwd` clean