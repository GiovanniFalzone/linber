/*-------------------------------------------------------------------------------------------------------
	This is a simple Server application using the linber_worker class to implement a Service
--------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>

#include "Calculator_service_worker.h"

#define REQUEST_EXEC_TIME		2
#define SERVICE_PERIOD			100
#define SERVICE_RPP				2		// Request Per Period


using namespace std;

Calculator_service_worker **calculator_workers;
char *str_uri = "org.calculator\0";
int uri_len = strlen(str_uri);
unsigned long service_token;
int num_workers = 3;

void sig_handler(int signo){
  if (signo == SIGINT){
    printf("received SIGINT\n");
	for(int i=0; i<num_workers; i++){
		calculator_workers[i]->terminate_worker();
	}
	linber_destroy_service(str_uri, uri_len, service_token);
  }
}

int main(){
	if(signal(SIGINT, sig_handler) == SIG_ERR){
		printf("can't catch SIGINT\n");
		return -1;
	}

	linber_init();
	linber_register_service(str_uri, uri_len, REQUEST_EXEC_TIME, num_workers, &service_token, SERVICE_PERIOD, SERVICE_RPP);

	calculator_workers = (Calculator_service_worker**)malloc(sizeof(Calculator_service_worker*)*num_workers);
	for(int i=0; i<num_workers; i++){
		calculator_workers[i] = new Calculator_service_worker(str_uri, service_token);
	}
	for(int i=0; i<num_workers; i++){
		calculator_workers[i]->join_worker();
	}
	for(int i=0; i<num_workers; i++){
		delete calculator_workers[i];
	}

	linber_exit();

	return 0;
}
