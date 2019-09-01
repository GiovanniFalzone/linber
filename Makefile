.DEFAULT_GOAL := help

.PHONY:
all: linber tests

.PHONY:
linber:
	cd driver && make clean && make all

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
monitor:
	./test/linber_monitor/linber_monitor 100

.PHONY:
clean:
	cd driver && make clean
	cd test && make clean

.PHONY:
help:
	@echo "[all] | [linber] | [tests] | [insmod] | [rmmod] | [updatemod] | [clean]"
