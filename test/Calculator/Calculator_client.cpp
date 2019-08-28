#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Calculator.pb.h"
#include "../../libs/linberServiceWorker.h"

using namespace std;

class calculator_client{
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;
	char *service_uri;
	int service_uri_len;
	int request_len;
	int response_len;
	char* request;
	char * response;
	boolean response_shm_mode;

	public:
	calculator_client(){
		service_uri = "org.calculator\0";
		service_uri_len = strlen(service_uri);
		linber_init();
	}

	~calculator_client(){
		linber_exit();
	}

	float sum(float a, float b){
		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::SUM);
		request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_request_service(	service_uri, service_uri_len,				\
								10, request, request_len,					\
								&response, &response_len, &response_shm_mode);

		response_msg.ParseFromArray(response, response_len);
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
		return response_msg.result();
	}

	float difference(float a, float b){
		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::DIFFERENCE);
		request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_request_service(	service_uri, service_uri_len,				\
								10, request, request_len,					\
								&response, &response_len, &response_shm_mode);

		response_msg.ParseFromArray(response, response_len);
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
		return response_msg.result();
	}

	float product(float a, float b){
		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::PRODUCT);
		request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_request_service(	service_uri, service_uri_len,				\
								10, request, request_len,					\
								&response, &response_len, &response_shm_mode);

		response_msg.ParseFromArray(response, response_len);
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
		return response_msg.result();
	}

	float division(float a, float b){
		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::DIVISION);
		request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_request_service(	service_uri, service_uri_len,				\
								10, request, request_len,					\
								&response, &response_len, &response_shm_mode);

		response_msg.ParseFromArray(response, response_len);
		linber_request_service_clean(request, FALSE, response, response_shm_mode);
		return response_msg.result();
	}
};

int main(){
	calculator_client calc;

	for(float i=0; i<10; i++){
		printf("(%f %c %f) = ", i, '+', i);
		cout << calc.sum(i, i) << endl;
		printf("(%f %c %f) = ", i, '-', i);
		cout << calc.difference(i, i) << endl;
		printf("(%f %c %f) = ", i, '*', i);
		cout << calc.product(i, i) << endl;
		printf("(%f %c %f) = ", i, '/', i);
		cout << calc.division(i, i) << endl;
		cout << "--------------------------------\n" << endl;
	}

	return 0;
}