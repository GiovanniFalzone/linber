#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../libs/linber_service_api.h"
#include <sys/time.h>


#define DEFAULT_SERVICE_URI	"org.service\0"
char *service_uri;
int uri_len;

int get_service_time(unsigned int req_size, int blocking, unsigned int *micros){
	char *service_request;
	int service_request_len;
	char *service_response;
	int service_response_len;
	struct timeval start, end;
	unsigned long passed_micros = 0, min_time_micros = 1000000;
	int ret = 0;
	unsigned long token;

	service_request = malloc(req_size + 1);
	if(service_request == NULL){
		printf("no memory");
		return -1;
	}
	memset(service_request, 'X', req_size);
	service_request[req_size] = '\0';
	service_request_len = req_size + 1;

	for(int i=0; i<1000; i++){
		gettimeofday(&start, NULL);
		if(blocking == 1){
			ret = linber_request_service(service_uri, uri_len, 1, service_request, service_request_len, &service_response, &service_response_len);
		} else {
			ret = linber_request_service_no_blocking(service_uri, uri_len, 1, service_request, service_request_len, &token);
			if(ret >= 0){
				ret = linber_request_service_get_response(service_uri, uri_len, &service_response, &service_response_len, &token);
			}
		}
		if(ret < 0){
			printf("request aborted\n");
		} else {
			gettimeofday(&end, NULL);
			passed_micros = (end.tv_usec - start.tv_usec);
			if(passed_micros < min_time_micros){
				min_time_micros = passed_micros;
			}
		}
	}
	*micros = min_time_micros;
	free(service_request);
	if(ret >= 0){
		free(service_response);
	}
	return 0;
}

void save_in_csv(unsigned int mat[][2], int n){
	FILE *fp;
	int i,j;
    char file_name[64];

    printf("\n Enter the filename :\n");
	scanf("%[^\n]%*c", file_name);

	fp=fopen(strcat(file_name, ".csv"), "w+");

	fprintf(fp, "Size, Micros\n");
	for(i=0; i<n; i++){
		for(j=0; j<2; j++){
			if(j < 2-1){
				fprintf(fp, "%u,", mat[i][j]);
			} else {
				fprintf(fp, "%u\n", mat[i][j]);
			}
		}
	}
	fclose(fp);
}


int main(int argc,char* argv[]){
	unsigned int min_size = 1;
	unsigned int size=min_size;
	int iterations = 10;
	unsigned int microsec;
	unsigned int mat[iterations][2];	//size, time

	service_uri = DEFAULT_SERVICE_URI;
	uri_len = strlen(service_uri);
	if(argc >= 2){
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
		uri_len = strlen(service_uri);
	}
	if(argc >= 3){
		int n = atoi(argv[2]);
		if(n > 0){
			min_size = n;
		}
	}
	if(argc >= 4){
		int n = atoi(argv[3]);
		if(n > 0){
			iterations = n;
		}
	}

	linber_init();
	printf("Running efficiency test on service %s\n", service_uri);
	for(int i=0; i<iterations; i++, size = size << 1){
		get_service_time(size, 1, &microsec);
		mat[i][0] = size;
		mat[i][1] = microsec;
		printf("Iteration:%d, size:%u, time: microsec:%u\n", i, mat[i][0], mat[i][1]);
	}
	linber_exit();

	save_in_csv(mat, iterations);
}
 
