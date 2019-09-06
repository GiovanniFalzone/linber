#include "Calculator_service_worker.h"

Calculator_service_worker::Calculator_service_worker(char * service_uri, unsigned long service_token)	\
					: linberServiceWorker(service_uri, service_token)
{
}

float Calculator_service_worker::sum(float a, float b){
	return a + b;
}

float Calculator_service_worker::difference(float a, float b){
	return a - b;
}

float Calculator_service_worker::product(float a, float b){
	return a * b;
}

float Calculator_service_worker::division(float a, float b){
	float ret = 0;
	if(a>0 && b>0){
		ret = a/b;
	}
	return ret;
}

float Calculator_service_worker::power(float a, float b){
	return pow(a, b);
}

float Calculator_service_worker::square_root(float a){
	return sqrt(a);
}

float Calculator_service_worker::compute(unsigned int operation, float a, float b){
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

void Calculator_service_worker::execute_job() {
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

	response = (char*)malloc(response_len);		// don't free this, the parent class will do that
	response_msg.SerializeToArray(response, response_len);
}
