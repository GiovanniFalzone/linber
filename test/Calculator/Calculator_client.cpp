#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Calculator.pb.h"
#include "../../libs/linberServiceWorker.h"

using namespace std;

int main(){
	Calculator::Calculator_msg msg;
	msg.set_operand_a(1);
	msg.set_operand_b(2);
	msg.set_operation(Calculator::SUM);

	cout << msg.operation() <<"("<< msg.operand_a() <<","<< msg.operand_b() <<")"<< endl;
	string str_msg = string(msg.ByteSize(), '\0');
	msg.SerializeToString(&str_msg);
	char * result;
	char * params = new char[str_msg.size() + 1];
	copy(str_msg.begin(), str_msg.end(), params);
	params[str_msg.size()] = '\0';

	int result_len;

	linber_init();
	linber_request_service("org.calculator1", sizeof("org.calculator1"), 10, params, str_msg.size(), result, &result_len);
	linber_exit();

	delete[] params;
	return 0;
}