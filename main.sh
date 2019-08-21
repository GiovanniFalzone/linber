#!/bin/sh
cmd="${1}" 
case ${cmd} in 
	makemodule)
		sh -c 'cd driver && make clean && make all'
		;;

	maketest)
		sh -c 'cd test && make clean && make all'
		;;

	makeall)
		sh -c 'cd driver && make clean && make all'
		sh -c 'cd test && make clean && make all'
		;;  

	clean)
		sh -c 'cd driver && make clean'
		sh -c 'cd test && make clean'
		;; 

	insmod)
		sh -c 'sudo insmod driver/linber.ko'
		;; 

	rmmod)
		sh -c 'sudo rmmod linber'
		;; 

	updatemod)
		sh -c 'cd driver && make all && sudo rmmod linber && sudo insmod linber.ko'
		;;

   *)  
	  echo "`basename ${0}`:usage: [makeall] | [makemodule] | [maketest] | [clean] | [insmod] | [rmmod] | [updatemod]" 
	  exit 1
	  ;; 
esac