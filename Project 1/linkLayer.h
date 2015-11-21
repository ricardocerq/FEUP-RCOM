#ifndef LINK_LAYER_H
#define LINK_LAYER_H

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
#include <limits.h>
#include "constants.h"

//initialize the application layer
int initLinkLayer(int port, int baudrate, int timeout, int numTransmissions, int role, int segmentSize, float errorProbability);

//free resources used by the application layer
int closeLinkLayer();

//establish the connection
int llopen();

//exit connection
int llclose();

//send a packet of information to the receiver
int llwrite(unsigned char* buffer, int length);

//receive a packet of information
int llread(unsigned char* buffer);

//display the stats referring to the link layer
void printLinkLayerStats();


struct linkLayerStats{
	int informationFrameNumber;
	int timeoutNumber;
	int rejNumber;
	int framesDiscarded;
};

struct linkLayer{
	char port[32];
	int baudRate;
	unsigned int sequenceNumber;
	unsigned int timeout;
	unsigned int numTransmissions;
	unsigned int attempts;
	unsigned char* receptionFrame;
	int receptionSize;
	unsigned char* transmissionFrame;
	int transmissionSize;
	int role;
	int fd;
	int giveUp;
	int maxSize;
	float errorProbability;
};


#endif