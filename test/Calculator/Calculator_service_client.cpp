#include "Calculator_service_client.h"

Calculator_service_client::Calculator_service_client() : linberServiceClient(SERVICE_NAME, sizeof(SERVICE_NAME)){
}

float Calculator_service_client::sum(float a, float b){
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

	linber_sendRequest(request, request_len, &response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	free(request);

	return response_msg.result();
}

float Calculator_service_client::difference(float a, float b){
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

	linber_sendRequest(request, request_len, &response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	free(request);

	return response_msg.result();
}

float Calculator_service_client::product(float a, float b){
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

	linber_sendRequest(request, request_len, &response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	free(request);

	return response_msg.result();
}

float Calculator_service_client::division(float a, float b){
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

	linber_sendRequest(request, request_len, &response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	free(request);

	return response_msg.result();
}

float Calculator_service_client::pow(float a, float b){
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
	linber_sendRequest_shm(&response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	detach_shm(request);

	return response_msg.result();
}

float Calculator_service_client::sqrt(float a){
	char *request;
	int response_len;
	char *response;

	request_msg.Clear();
	request_msg.set_operand_a(a);
	request_msg.set_operation(Calculator::SQRT);
	int request_len = request_msg.ByteSize();
	request = (char*)malloc(request_len);
	request_msg.SerializeToArray(request, request_len);

	linber_sendRequest(request, request_len, &response, &response_len, true, REL_DEADLINE);

	response_msg.ParseFromArray(response, response_len);

	linber_end_operation();
	free(request);

	return response_msg.result();
}
