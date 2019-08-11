#!/bin/sh
cmd="${1}" 
case ${cmd} in 
	makeall)
		sh -c 'cd driver && make'
		sh -c 'cd test && make'
		;;  
	clean)
		sh -c 'cd driver && make clean'
		sh -c 'cd test && make clean'
		;; 
	insmod)
		sh -c 'sudo insmod driver/linber.ko && sudo chmod 755 /dev/linber'
		;; 
	rmmod)
		sh -c 'sudo rmmod linber'
		;; 

	updatemod)
		sh -c 'cd driver && make clean && make && sudo rmmod linber && sudo insmod linber.ko && sudo chmod 755 /dev/linber'
		;;

	runtest)
		test/client
		;;

   *)  
	  echo "`basename ${0}`:usage: [makeall] | [clean] | [insmod] | [rmmod] | [runtest]" 
	  exit 1
	  ;; 
esac