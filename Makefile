.PHONY:
all: linber test

.PHONY:
linber:
	cd driver && make all

.PHONY:
tests:
	cd test && make all

.PHONY:
insmod:
	sudo insmod driver/linber.ko

.PHONY:
rmmod:
	sudo rmmod linber

.PHONY:
updatemod: linber rmmod insmod

.PHONY:
help:
	@echo "[all] | [linber] | [tests] | [insmod] | [rmmod] | [updatemod] | [clean] "

.PHONY:
clean:
	cd driver && make clean
	cd test && make clean
