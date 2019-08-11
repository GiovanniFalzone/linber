#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "../libs/linber_service_api.h"

#define SERVICE_NAME	"org.server.func.1\0"

#define REL_DEADLINE		10
#define SERVICE_EXECTIME	5

int main(){
	char *service_name = SERVICE_NAME;
	printf("Running client test\n");
	linber_init();
	linber_register_service(SERVICE_NAME, sizeof(SERVICE_NAME), SERVICE_EXECTIME);
	linber_request_service(SERVICE_NAME, sizeof(SERVICE_NAME), REL_DEADLINE);
	linber_exit();
}