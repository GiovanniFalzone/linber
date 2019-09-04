/*-------------------------------------------------------------------------------------------------------
	This is a Client requesting a service with non blocking procedure, it pass the request as shared memory
--------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../../libs/linber_service_api.h"
#include <time.h>

#define DEFAULT_SERVICE_URI	"org.service\0"

#define ITERATION_FOR_MIN	1000
#define MAX_SIZE			1<<30

char *service_uri;
int uri_len;

int get_service_time(unsigned int req_size, unsigned int *min_nsec, unsigned int *avg_nsec){
	char *request;
	int request_len;
	char *response;
	boolean response_shm_mode = FALSE;
	int response_len;
	struct timespec start, end;
	unsigned long passed_nanos = 0, min_time_nanos = -1, avg_nanos = 0;
	int ret = 0;
	key_t request_shm_key;
	int request_shm_id;
	static int low_id = 0;

	for(int i=0; i<ITERATION_FOR_MIN; i++){
		*avg_nsec = 0;
		*min_nsec = 0;

		request_len = req_size + 1;
		request = create_shm_from_filepath(".", low_id++, request_len, &request_shm_key, &request_shm_id);

		if(request == NULL){
			printf("Error in shared memory allocation\n");
			return -1;
		}

		if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
			perror("clock gettime error\n");
		}

		ret = linber_request_service_shm(	service_uri, uri_len,						\
											0, request_shm_key, request_len,			\
											&response, &response_len, &response_shm_mode);


		if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
			perror("clock gettime error\n");
		}

		if(ret < 0){
			printf("aborted\n");
			return ret;
		}

		linber_request_service_clean(response, response_shm_mode);
		detach_shm(request);

		passed_nanos = SEC_TO_nSEC(end.tv_sec) - SEC_TO_nSEC(start.tv_sec);
		passed_nanos += (end.tv_nsec - start.tv_nsec);

		avg_nanos += passed_nanos;
		if(passed_nanos < min_time_nanos){
			min_time_nanos = passed_nanos;
		}
	}
	*avg_nsec = avg_nanos/ITERATION_FOR_MIN;
	*min_nsec = min_time_nanos;
	return 0;
}

void save_in_csv(unsigned int mat[][3], int n){
	FILE *fp;
	int i,j;
    char file_name[64];

    printf("\n Enter the filename :\n");
	scanf("%[^\n]%*c", file_name);

	fp=fopen(strcat(file_name, ".csv"), "w+");

	fprintf(fp, "Size, Min_ns, Avg_ns\n");
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
	unsigned int min_nsec, avg_nsec;

	service_uri = DEFAULT_SERVICE_URI;
	uri_len = strlen(service_uri);
	if(argc >= 2){		// SERVICE URI
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
		uri_len = strlen(service_uri);
	}
	if(argc >= 3){		// MIN SIZE
		int n = atoi(argv[2]);
		if(n > 0){
			min_size = n;
		}
	}
	if(argc >= 4){		// HOW MANY TIMES TO MULTIPLY MIN SIZE BY 2
		int n = atoi(argv[3]);
		if(n > 0){
			iterations = n;
		}
	}

	size=min_size;
	iterations++;
	unsigned int mat[iterations][3];	//size, min, avg

	linber_init();
	printf("Running efficiency test on service %s\n", service_uri);
	for(int i=0; i<iterations; i++, size = size << 1){
		if(size > MAX_SIZE){
			printf("maximum allowed dimension %d\n", MAX_SIZE);
			break;
		}

		get_service_time(size, &min_nsec, &avg_nsec);
		mat[i][0] = size;
		mat[i][1] = min_nsec;
		mat[i][2] = avg_nsec;
		printf("Iteration:%d, size:%u, min_us: %u avg_us: %u\n", i, mat[i][0], mat[i][1]/1000, mat[i][2]/1000);
	}
	linber_exit();

	save_in_csv(mat, iterations);
}
 