#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>
#include "constants.h"
#include "applicationLayer.h"

int baudRates[] = {BAUDRATE, BAUDRATE_300, BAUDRATE_600, BAUDRATE_1200, BAUDRATE_2400, BAUDRATE_4800, BAUDRATE_9600, BAUDRATE_19200, BAUDRATE_38400, BAUDRATE_57600, BAUDRATE_115200 };


static void processArgs(char* argv[], int args, int* baudrate, int* timeout, int* numTransmissions, int* segmentSize, float* errorProbability, char** filename){
	int i = 0;
	for(; i < args; i++){
		if(strcmp(argv[i], "-t") == 0){
			i++;
			*timeout = strtol(argv[i], NULL, 10);
		}else if(strcmp(argv[i], "-n") == 0){
			i++;
			*numTransmissions = strtol(argv[i], NULL, 10);
		}else if(strcmp(argv[i], "-s") == 0){
			i++;
			*segmentSize = strtol(argv[i], NULL, 10);
		}else if(strcmp(argv[i], "-b") == 0){
			i++;
			*baudrate = strtol(argv[i], NULL, 10);
		}else if(strcmp(argv[i], "-e") == 0){
			i++;
			*errorProbability = strtof(argv[i], NULL);
		}else if(strcmp(argv[i], "-f") == 0){
			i++;
			*filename = argv[i];
		}

	}
}


int main(int argc, char* argv[]){	
	srand(time(NULL));
	if(argc < 2 ){
		printf("Wrong Number of Arguments, <port> <-t timeout> <-n numTransmissions> <-s segmentSize> <-b baudrate> <-e errorProbability> <-f file> \n");
	}

	int port = strtol (argv[1], NULL, 10);

	int baudrate = 10; int timeout = 1; int numTransmissions = 10; int segmentSize = 256;
	float errorProbability = 0;
	char* filename = NULL;

	processArgs(argv + 2, argc - 2, &baudrate, &timeout, &numTransmissions, &segmentSize, &errorProbability, &filename);
	if(baudrate < 0 ){

		baudrate = 0;
		printf("Invalid Baudrate: Choosing %d\n", baudrate);
	}
	if(baudrate > 10){
		baudrate = 10;
		printf("Invalid Baudrate: Choosing %d\n", baudrate);
	}

	baudrate = baudRates[baudrate];
	int role;
	if(filename == NULL){
		role = 0;
	}
	else role = 1;

	printf("Settings\n");
	printf("\tTimeout: %d\n", timeout);
	printf("\tNumber of Transmissions: %d\n", numTransmissions);
	printf("\tSegment Size: %d\n", segmentSize);
	printf("\tBaudrate: %d\n", baudrate);
	printf("\tError Probability: %f\n", errorProbability);
	if(filename != NULL)
		printf("\tFilename: %s\n", filename);

	int fd = initAppLayer(port, baudrate, timeout, numTransmissions, role, segmentSize, errorProbability);


	if(fd < 0){
		printf("Could not Open Port\n");
		exit(1);
	}

	int failure = 0;

	if(role){
		/*int fd = open("test.txt", O_WRONLY|O_CREAT, 0666);
		int i = 0;
		unsigned char num;
		for(;i < 256; i++){
			num = (unsigned char)i;
			write(fd, &num, 1);
		}
		close(fd);*/
		failure = sendFile(filename);
	
	}else {
		failure = receiveFile();
	}
	if(failure){
		printf("Operation Failed\n");
	}else printf("Operation Sucessful\n");
	printApplicationLayerStats();

	closeAppLayer();

	return 0;
}