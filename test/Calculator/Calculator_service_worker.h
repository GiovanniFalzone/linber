#include <math.h>

#include "Calculator.pb.h"
#include "linberServiceWorker.h"

class Calculator_service_worker : public linberServiceWorker{
	Calculator::Calculator_request request_msg;
	Calculator::Calculator_response response_msg;

	public:
	Calculator_service_worker(char * service_uri, unsigned long service_token);
	float sum(float a, float b);
	float difference(float a, float b);
	float product(float a, float b);
	float division(float a, float b);
	float power(float a, float b);
	float square_root(float a);
	float compute(unsigned int operation, float a, float b);
	void execute_job()override;
};