#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Calculator.pb.h"
#include "../../libs/linberServiceWorker.h"

using namespace std;

class calculator_service : public linberServiceWorker{
	Calculator::Calculator_msg msg;

	public:
	calculator_service(char * service_uri, unsigned long service_token, int service_params_len, int service_result_len) \
						: linberServiceWorker(service_uri, service_token, service_params_len, service_result_len){

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
	float compute(unsigned int operation, float a, float b){
		float ret = 0;
		switch(operation){
			case 0:
				ret = sum(a, b);
				break;
			case 1:
				ret = difference(a, b);
				break;
			case 2:
				ret = product(a, b);
				break;
			case 3:
				ret = division(a, b);
				break;
			default:
				break;
		}
		return ret;
	}

	void execute_job(string str_msg){
		msg.ParseFromString(str_msg);
		cout << msg.operation() <<"("<< msg.operand_a() <<","<< msg.operand_b() <<")"<< endl;
		cout << compute(msg.operation(), msg.operand_a(), msg.operand_b()) << endl;
	}
};

int main(){
	string str_uri("org.calculator1");
	unsigned long service_token;
	linber_init();
	linber_register_service("org.calculator1", sizeof("org.calculator1"), 100, 1, &service_token);
	Calculator::Calculator_msg msg;
	calculator_service *calculator1 = new calculator_service("org.calculator1", service_token, msg.ByteSize(), msg.ByteSize());
	calculator_service *calculator2 = new calculator_service("org.calculator1", service_token, msg.ByteSize(), msg.ByteSize());
	calculator_service *calculator3 = new calculator_service("org.calculator1", service_token, msg.ByteSize(), msg.ByteSize());

//	linber_destroy_service("org.calculator1", sizeof("org.calculator1"), service_token);

	delete calculator1;
	delete calculator2;
	delete calculator3;

	linber_exit();
	return 0;
}
