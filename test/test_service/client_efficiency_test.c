#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../libs/linber_service_api.h"
#include <sys/time.h>

#define ITERATION_SAME_REQUEST	1000


#define DEFAULT_SERVICE_URI	"org.service\0"
char *service_uri;
int uri_len;

int get_service_time(unsigned int req_size, int blocking, unsigned int *min_micros, unsigned int *avg_micros){
	char *request;
	int request_len;
	char *response;
	boolean response_shm_mode = FALSE;
	int response_len;
	struct timeval start, end;
	unsigned long passed_micros = 0, min_time_micros = 1000000, avg_time_micros = 0;
	int ret = 0;
	unsigned long token;

	for(int i=0; i<ITERATION_SAME_REQUEST; i++){
		request_len = req_size + 1;
		request = malloc(request_len);
		if(request == NULL){
			printf("no memory");
			return -1;
		}

		#ifdef DEBUG
			memset(request, 'X', request_len);
			request[request_len - 1] = '\0';
		#endif

		gettimeofday(&start, NULL);
		if(blocking == 1){
			ret = linber_request_service(	service_uri, uri_len,							\
											1, request, request_len,						\
											&response, &response_len, &response_shm_mode);
		} else {
			ret = linber_request_service_no_blocking(	service_uri, uri_len,		\
														1, request, request_len,	\
														&token);
			if(ret >= 0){
				ret = linber_request_service_get_response(	service_uri, uri_len,							\
															&response, &response_len, &response_shm_mode,	\
															&token);
			}
		}

		linber_request_service_clean(request, FALSE, response, response_shm_mode);

		if(ret < 0){
			printf("request aborted\n");
		} else {
			gettimeofday(&end, NULL);
			passed_micros = (end.tv_usec - start.tv_usec);
			avg_time_micros += passed_micros;
			if(passed_micros < min_time_micros){
				min_time_micros = passed_micros;
			}
		}
	}
	*avg_micros = avg_time_micros/ITERATION_SAME_REQUEST;
	*min_micros = min_time_micros;
	return 0;
}

void save_in_csv(unsigned int mat[][3], int n){
	FILE *fp;
	int i,j;
    char file_name[64];

    printf("\n Enter the filename :\n");
	scanf("%[^\n]%*c", file_name);

	fp=fopen(strcat(file_name, ".csv"), "w+");

	fprintf(fp, "Size, Micros\n");
	for(i=0; i<n; i++){
		for(j=0; j<3; j++){
			if(j < 3-1){
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
	unsigned int min_micros, avg_micros;

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
			if(iterations >= 22){
				printf("maximum allowed dimension 4Mb\n");
				iterations = 21;
			}
		}
	}

	size=min_size;
	iterations++;
	unsigned int mat[iterations][3];	//size, min, avg

	linber_init();
	printf("Running efficiency test on service %s\n", service_uri);
	for(int i=0; i<iterations; i++, size = size << 1){
		get_service_time(size, 1, &min_micros, &avg_micros);
		mat[i][0] = size;
		mat[i][1] = min_micros;
		mat[i][2] = avg_micros;
		printf("Iteration:%d, size:%u, min: %u avg: %u\n", i, mat[i][0], mat[i][1], mat[i][2]);
	}
	linber_exit();

	save_in_csv(mat, iterations);
}
 