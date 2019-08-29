#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Calculator.pb.h"
#include "../../libs/linberClient.h"

using namespace std;

class calculator_client : public linberClient{
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;

	public:
	calculator_client() : linberClient("org.calculator\0", sizeof("org.calculator\0")){
	}

	float sum(float a, float b){
		char *request;
		int response_len;
		char *response;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::SUM);
		int request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_sendRequest(request, request_len, &response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}

	float difference(float a, float b){
		char *request;
		int response_len;
		char *response;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::DIFFERENCE);
		int request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_sendRequest(request, request_len, &response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}

	float product(float a, float b){
		char *request;
		int response_len;
		char *response;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::PRODUCT);
		int request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_sendRequest(request, request_len, &response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}

	float division(float a, float b){
		char *request;
		int response_len;
		char *response;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::DIVISION);
		int request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_sendRequest(request, request_len, &response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}

	float pow(float a, float b){
		char *request;
		char *response;
		int request_len;
		int response_len;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operand_b(b);
		request_msg.set_operation(Calculator::POW);
		
		request_len = request_msg.ByteSize();
		linber_create_shm(&request, request_len);
		request_msg.SerializeToArray(request, request_len);
		linber_sendRequest_shm(&response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}

	float sqrt(float a){
		char *request;
		int response_len;
		char *response;

		request_msg.Clear();
		request_msg.set_operand_a(a);
		request_msg.set_operation(Calculator::SQRT);
		int request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);
		request_msg.SerializeToArray(request, request_len);

		linber_sendRequest(request, request_len, &response, &response_len, true);

		response_msg.ParseFromArray(response, response_len);
		linber_end_operation();

		return response_msg.result();
	}
};

int main(){
	calculator_client calc;

	for(float i=0; i<10; i++){
		cout << "--------------"<< i <<"------------------" << endl;
		printf("(%f %c %f) = ", i, '+', i);
		cout << calc.sum(i, i) << endl;
		printf("(%f %c %f) = ", i, '-', i);
		cout << calc.difference(i, i) << endl;
		printf("(%f %c %f) = ", i, '*', i);
		cout << calc.product(i, i) << endl;
		printf("(%f %c %f) = ", i, '/', i);
		cout << calc.division(i, i) << endl;
		printf("(%f %c %f) = ", i, '^', i);
		cout << calc.pow(i, i) << endl;
		printf("(\\/%f) = ", i);
		cout << calc.sqrt(i) << endl;
	}

	return 0;
}