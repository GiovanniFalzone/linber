#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <signal.h>
#include <math.h>
#include "Calculator.pb.h"
#include "../../libs/linberServiceWorker.h"

using namespace std;

class calculator_service : public linberServiceWorker{
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;

	public:
	calculator_service(char * service_uri, int service_id, unsigned long service_token)	\
					 : linberServiceWorker(service_uri, service_id, service_token){
	}

	float sum(float a, float b){
		return a + b;
	}
	float difference(float a, float b){
		return a - b;
	}
	float product(float a, float b){
		return a * b;
	}
	float division(float a, float b){
		float ret = 0;
		if(a>0 && b>0){
			ret = a/b;
		}
		return ret;
	}
	float power(float a, float b){
		return pow(a, b);
	}
	float square_root(float a){
		return sqrt(a);
	}

	float compute(unsigned int operation, float a, float b){
		float ret = 0;
		switch(operation){
			case Calculator::SUM:
				ret = sum(a, b);
				break;
			case Calculator::DIFFERENCE:
				ret = difference(a, b);
				break;
			case Calculator::PRODUCT:
				ret = product(a, b);
				break;
			case Calculator::DIVISION:
				ret = division(a, b);
				break;
			case Calculator::POW:
				ret = power(a, b);
				break;
			case Calculator::SQRT:
				ret = square_root(a);
				break;

			default:
				break;
		}
		return ret;
	}

	void execute_job() override {
		unsigned int op;
		float a;
		float b;
		float result;

		request_msg.ParseFromArray(request, request_len);
		op = request_msg.operation();
		a = request_msg.operand_a();
		b = request_msg.operand_b();

		result = compute(op, a, b);
		response_msg.set_result(result);
		response_len = response_msg.ByteSize();

		response = (char*)malloc(response_len);
		response_msg.SerializeToArray(response, response_len);
	}
};

calculator_service **calculator_workers;
int num_workers = 3;
char *str_uri = "org.calculator\0";

int uri_len = strlen(str_uri);
unsigned long service_token;

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
	int service_id;
	if(signal(SIGINT, sig_handler) == SIG_ERR){
		printf("can't catch SIGINT\n");
		return -1;
	}

	linber_init();
	linber_register_service(str_uri, uri_len, &service_id, 1, num_workers, &service_token);

	calculator_workers = (calculator_service**)malloc(sizeof(calculator_service*)*num_workers);
	for(int i=0; i<num_workers; i++){
		calculator_workers[i] = new calculator_service(str_uri, service_id, service_token);
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
