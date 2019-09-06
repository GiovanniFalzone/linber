/*-------------------------------------------------------------------------------------------------------
	This is a simple Client application using the linber_client class to implement a client of a service
--------------------------------------------------------------------------------------------------------*/

#include "Calculator.pb.h"
#include "linberServiceClient.h"

#define REL_DEADLINE 100
#define SERVICE_NAME "org.calculator\0"


class Calculator_service_client : public linberServiceClient{
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;

	public:
	Calculator_service_client();
	float sum(float a, float b);

	float difference(float a, float b);
	float product(float a, float b);
	float division(float a, float b);
	float pow(float a, float b);
	float sqrt(float a);
};
