#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Calculator.pb.h"
#include "../../libs/linberServiceWorker.h"

using namespace std;

int main(){
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;
	int request_len;
	int response_len;
	char* request;
	char * response;

	linber_init();

	for(int i=0; i < 100; i++){
		request_msg.Clear();
		request_msg.set_operand_a(i);
		request_msg.set_operand_b(i);
		request_msg.set_operation(Calculator::PRODUCT);

		request_len = request_msg.ByteSize();
		request = (char*)malloc(request_len);

		cout << request_msg.operation() <<"("<< request_msg.operand_a() <<","<< request_msg.operand_b() <<")" << "len:" << request_len << endl;
		request_msg.SerializeToArray(request, request_len);

		linber_request_service("org.calculator", sizeof("org.calculator1"), 100, request, request_len, &response, &response_len);

		response_msg.ParseFromArray(response, response_len);
		cout << "Result: "<< response_msg.result() << endl;

		free(request);
		free(response);
	}

	linber_exit();
	return 0;
}