#ifndef APP_LAYER_H
#define APP_LAYER_H

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
#include "linkLayer.h"

struct applicationLayer{
	unsigned int sequenceNumber;
	int role;
	unsigned char* buffer;
	int buffersize;
	int segmentSize;
};

//send the file located at filepath to the receiver
int sendFile(char * filepath);

//receive the file being transmitted
int receiveFile();

//initialize the application layer
int initAppLayer(int port, int baudrate, int timeout, int numTransmissions, int role, int segmentSize, float errorProbability);

//release resources used by the application layer
int closeAppLayer();

//print the stats of the application layer
void printApplicationLayerStats();


#endif