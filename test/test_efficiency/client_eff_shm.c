/*-------------------------------------------------------------------------------------------------------
	This is a Client requesting a service with non blocking procedure, it pass the request as shared memory
--------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../../libs/linber_service_api.h"
#include <time.h>

#define DEFAULT_SERVICE_URI			"org.service\0"
#define RESULTS_FILE_NAME			"shm_eff_test_1B_to_1GB"
#define ITERATION_FOR_SAME_REQUEST	1000
#define MAX_MULTIPLICATIONS			30									
//---------------------------------------------------------
#define FILE_NAME 		RESULTS_FILE_NAME ".csv"

#define LABEL_SIZE "size\0"

#define MAX_SIZE					1<<MAX_MULTIPLICATIONS				// Max size = 2^30 = 1GB
#define MAX_ROW						MAX_MULTIPLICATIONS + 1				// first row contain labels
#define MAX_COLUMN					ITERATION_FOR_SAME_REQUEST + 1		// first column contain the size



char *service_uri;
int uri_len;
unsigned int result_matrix[MAX_ROW][MAX_COLUMN];	//row = [size, it_1, it_2, ... it_1000]


unsigned int execute_request(unsigned int request_len){
	char *request;
	char *response;
	boolean response_shm_mode = FALSE;
	int response_len;
	struct timespec start, end;
	unsigned long passed_nanos = 0;
	key_t request_shm_key;
	int request_shm_id;
	static int low_id = 0;
	int ret = 0;

	request = create_shm_from_filepath(".", low_id++, request_len, &request_shm_key, &request_shm_id);

	if(request == NULL){
		printf("Error in shared memory allocation\n");
		return -1;	// return the maximum value
	}

	if(clock_gettime(CLOCK_MONOTONIC, &start) == -1){
		perror("clock gettime error\n");
	}

//----------------------measuring----------------------------------
// blocking request, 0 as deadline so will use SCHED_FIFO with 99 as priority
	ret = linber_request_service_shm(	service_uri,					\
										uri_len,						\
										0,								\
										request_shm_key,				\
										request_len,					\
										&response,						\
										&response_len,					\
										&response_shm_mode				\
									);
//------------------end measuring----------------------------------

	if(clock_gettime(CLOCK_MONOTONIC, &end) == -1){
		perror("clock gettime error\n");
	}

	linber_request_service_clean(response, response_shm_mode);
	detach_shm(request);

	if(ret < 0){
		printf("aborted\n");
		return -1;	// return a maximum value
	}

	passed_nanos = SEC_TO_nSEC(end.tv_sec) - SEC_TO_nSEC(start.tv_sec);
	passed_nanos += (end.tv_nsec - start.tv_nsec);
	return passed_nanos;
}

void test_with_size(int row, unsigned int size){
	unsigned long passed_nanos = 0;
	unsigned int min_nsec = -1;

	for(int i=0; i < ITERATION_FOR_SAME_REQUEST; i++){
		passed_nanos = execute_request(size);
		if(passed_nanos < min_nsec){
			min_nsec = passed_nanos;
		}
		result_matrix[row][i+1] = passed_nanos;
	}
	result_matrix[row][0] = size;
	printf("Iteration:%d, size:%u, min_us: %u\n", row, size, min_nsec/1000);
}

void save_in_csv(){
	FILE *fp;
	int i,j;
	char str_num[8];
	char labels[sizeof(LABEL_SIZE) + ITERATION_FOR_SAME_REQUEST*(5)];		// #001,#002,...#010,...#100...#1000

	fp = fopen(FILE_NAME, "w+");
	strcpy(labels, LABEL_SIZE);
	for(i=1; i<=(MAX_COLUMN-1); i++){
		sprintf(str_num, ",#%d", i);
		strcat(labels, str_num);
	}
	strcat(labels, "\n\0");

	fprintf(fp, "%s", labels);
	for(i=0; i<MAX_ROW; i++){
		for(j=0; j<MAX_COLUMN; j++){
			if(j == (MAX_COLUMN-1)){
				fprintf(fp, "%u\n", result_matrix[i][j]);
			} else {
				fprintf(fp, "%u,", result_matrix[i][j]);
			}
		}
	}
	fclose(fp);
}

int main(int argc,char* argv[]){
	unsigned int size = 1;
	int i;

	service_uri = DEFAULT_SERVICE_URI;
	uri_len = strlen(service_uri);
	if(argc >= 2){							// SERVICE URI
		service_uri = malloc(strlen(argv[1])+1);
		strcpy(service_uri, argv[1]);
		uri_len = strlen(service_uri);
	}

	linber_init();
	printf("Running efficiency test on service %s\n", service_uri);
	for(i=0, size=1; i <= MAX_MULTIPLICATIONS; i++, size = size << 1){
		if(size > MAX_SIZE){
			printf("maximum allowed dimension %d\n", MAX_SIZE);
			break;
		}
		test_with_size(i, size);
	}
	linber_exit();

	save_in_csv();
}
 