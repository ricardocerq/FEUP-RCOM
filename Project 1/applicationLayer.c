#include "applicationLayer.h"
#include "constants.h"


static struct applicationLayer applicationLayerAttrs;

int initAppLayer(int port, int baudrate, int timeout, int numTransmissions, int role, int segmentSize, float errorProbability){
	applicationLayerAttrs.sequenceNumber = 0;
	applicationLayerAttrs.role = role;
	applicationLayerAttrs.buffersize = 0;
	applicationLayerAttrs.buffer = (unsigned char*) malloc(PACKET_SIZE(segmentSize) * sizeof(unsigned char));
	applicationLayerAttrs.segmentSize = segmentSize;
	return initLinkLayer(port, baudrate, timeout, numTransmissions, role, segmentSize, errorProbability);
}

int closeAppLayer(){
	if(applicationLayerAttrs.buffer != NULL){
		free(applicationLayerAttrs.buffer);
		applicationLayerAttrs.buffer = NULL;
	}
	return closeLinkLayer();
}

static int formDataPacket(unsigned char* buffer, unsigned char* data, int* size, unsigned int sequenceNumber){
	buffer[0] = C_DATA;
	buffer[1] = applicationLayerAttrs.sequenceNumber;
	
	unsigned char l2 = (*size)/256;
	unsigned char l1 = (*size)%256;
	buffer[2] = l2;
	buffer[3] = l1;
	memcpy((void*) buffer + 4, (void*) data, *size);
	*size += 4;
	return *size;
}

static int formControlPacket(unsigned char* buffer, int* size, int start, char* filename, unsigned int filesize){
	if(start)
		buffer[0] = C_START;
	else buffer[0] = C_END;
	int i = 0;
	buffer[1] = T_NAME;
	int length = strlen(filename);
	buffer[2] = length;
	//i+=2;
	memcpy((void*) buffer + 3, (void*) filename, length);
	//i+=buffer[i + 2];
	
	buffer[3 + length] = T_SIZE;
	buffer[3 + length + 1] = sizeof(filesize);
	//i+=2;
	memcpy((void*) buffer + 3 + length + 2, (void*) &filesize, buffer[3 + length + 1]);
	//i+=buffer[i + 2];
	*size = 3 + length + 2 + buffer[3 + length + 1];
	//printf("Control Packet size: %d\n", i);
	return i;	
}

static int mapFile(char* filepath, unsigned char** file, unsigned int* filesize){
	int fd = open(filepath, O_RDONLY);
	if(fd < 0){
		printf("mapFile(): Could not open file %s\n", filepath);
		return -1;
	}
	struct stat st;
	if(fstat(fd,&st) < 0){
        printf("mapFile(): fstat failed\n");
        return -1;
	}

    *filesize = st.st_size;
 	*file = NULL;
    if((*file = (unsigned char*) mmap(NULL, *filesize, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED){
        printf("mapFile(): mmap failed\n");
        return -1;
    }
    return 0;
}
static int unmapFile(unsigned char* file, unsigned int filesize){
	if( munmap(file,filesize) == -1){
	        printf("unmapFile(): munmap failed\n");
	        return -1;
	}
	return 0;
}

static int sendPacket(unsigned char* buffer, int size){
	if(llwrite(buffer, size))
		return 0;
	printf("sendPacket(): Connection Lost\n");
	/*printf("sendPacket(): Connection Lost. Reconnecting...\n");
	if(llopen())
		if(llwrite(buffer, size)){
			printf("sendPacket(): Connection reestablished\n");
			return 0;
		}
	printf("sendPacket(): Could not reconnect\n");*/
	return -1;
}

int sendFile(char * filepath){

	unsigned char* file;
	unsigned int filesize;
	if(mapFile(filepath, &file, &filesize)){
		printf("sendFile() : mapFile() failed\n");
		return -1;
	}
	
	unsigned int bytesLeft = filesize;
	unsigned int nextPacketSize;
	int position = 0;
	int failure = 0;
	if(!llopen()){
		printf("sendFile() : connection timed out, giving up :( 1\n");
		failure = 1;
	}
	if(!failure){
		char* filename = filepath + strlen(filepath);
		while(!(filename == filepath || *(filename-1) == '/')){filename--;}
		formControlPacket(applicationLayerAttrs.buffer, &applicationLayerAttrs.buffersize, 1, filename, filesize);
		if(sendPacket(applicationLayerAttrs.buffer, applicationLayerAttrs.buffersize)){
			printf("sendFile() : connection timed out, giving up :( 2\n");
			failure = 1;
		}
		if(!failure){
			printf("sendFile() : Sending packet containing %s, %d\n", filename, filesize);
			while(bytesLeft > 0){
				
				nextPacketSize = MIN(applicationLayerAttrs.segmentSize, bytesLeft);
				
				applicationLayerAttrs.buffersize = nextPacketSize;
				formDataPacket(applicationLayerAttrs.buffer, file + position, &applicationLayerAttrs.buffersize, applicationLayerAttrs.sequenceNumber);
				if(sendPacket(applicationLayerAttrs.buffer, applicationLayerAttrs.buffersize)){
					printf("sendFile() : connection timed out, giving up :( 3\n");
					failure = 1;
					break;
				}
				bytesLeft -= nextPacketSize;
				position += nextPacketSize;
				applicationLayerAttrs.sequenceNumber = (applicationLayerAttrs.sequenceNumber + 1) % 256;
				printf("Sending Data, %f%% complete\n", 100 - ((float) bytesLeft*100) / filesize);
			}
			if(!failure){
				formControlPacket(applicationLayerAttrs.buffer, &applicationLayerAttrs.buffersize, 0, filename, filesize);
				if(sendPacket(applicationLayerAttrs.buffer, applicationLayerAttrs.buffersize)){
					printf("sendFile() : connection timed out, giving up :( 4\n");
					failure = 1;
				}
			}
			if(!failure)
				llclose();
		}
	}
	if(unmapFile(file, filesize)){
		printf("sendFile() : unmapFile() failed\n");
		return -1;
	}
	return -failure;
}

int receivePacket(unsigned char* buffer, int* size){
	int numReceived = 0;
	if((numReceived = llread(buffer)) > 0){
		*size = numReceived;
		return 0;
	}
	printf("receivePacket(): Connection Lost\n");
	return -1;
}

static int extractFileInfo(unsigned char* buffer, char** filename, int* filesize){
	if(buffer[0] != C_START && buffer[0] != C_END){
		printf("Packet is not Control Packet\n");
		return -1;
	}
	if(buffer[1] != T_NAME){
		printf("First t is not name\n");
		return -1;
	}
	unsigned char filenamesize = buffer[2];
	*filename = (char *) malloc((filenamesize + 1)* sizeof(char));
	memcpy((void*)*filename, (void*) buffer + 3, filenamesize);
	(*filename)[filenamesize] = '\0';

	if(buffer[3 + filenamesize] != T_SIZE){
		printf("Second t is not size\n");
		return -1;
	}
	unsigned char filesizesize = buffer[4 + filenamesize];
	memcpy((void*)&(*filesize), (void*) buffer + filenamesize + 5, filesizesize);
	return 0;
}


int receiveFile(){
	int failure = 0;
	char* filename = NULL;
	int filesize = 0;
	char* filename2 = NULL;
	int filesize2 = 0;

	int numReceivedBytes = 0;
	if(!llopen()){
		printf("receiveFile() : connection timed out, giving up :( 1\n");
		failure = 1;
	}
	if(!failure){
		printf("receiveFile() : connection established\n");
		if(receivePacket(applicationLayerAttrs.buffer, &applicationLayerAttrs.buffersize)){
			printf("receiveFile() : connection timed out, giving up :( 2\n");
			printf("Invalid Packet Received ");
			//printFrame(applicationLayerAttrs.buffer, applicationLayerAttrs.buffersize);
			//printf("\n");
			failure = 1;
		}
		else {
			printf("receiveFile() : First packet received\n");
			if(extractFileInfo(applicationLayerAttrs.buffer, &filename, &filesize)){
				printf("Error in first packet\n");
				failure = 1;
			}else printf("receiveFile() : File name: %s, filesize: %d\n", filename, filesize);
		}
		if(!failure){
			
			int fd = open(filename, O_WRONLY | O_CREAT, 0666);
			if(fd < 0){
				printf("receiveFile(): Could not open file %s\n", filename);
				return -1;
			}
			int receivedType = -1;
			do {
				if(receivePacket(applicationLayerAttrs.buffer, &applicationLayerAttrs.buffersize)){
					printf("receiveFile() : connection timed out, giving up :( 3\n");
					failure = 1;
					break;
				}
				receivedType = applicationLayerAttrs.buffer[0];

				if(receivedType == C_DATA){
					
					if(applicationLayerAttrs.sequenceNumber != applicationLayerAttrs.buffer[1]){
						printf("Received Wrong Sequence Number\n");
						failure = 1;
						break;
					}

					int numBytes = applicationLayerAttrs.buffer[2] * 256 + applicationLayerAttrs.buffer[3];
					numReceivedBytes += numBytes;
					printf("Received Data Packet #%3d, %d/%d bytes, %f%% complete\n", applicationLayerAttrs.buffer[1], numReceivedBytes, filesize, ((float) numReceivedBytes*100) / filesize);
					int res = write(fd, applicationLayerAttrs.buffer + 4, numBytes);
					if(res != numBytes)
						printf("Error in Write\n");
					applicationLayerAttrs.sequenceNumber = (applicationLayerAttrs.sequenceNumber + 1) % 256;
				}else if(receivedType == C_START){
					printf("Received Start control packet in main loop\n");
				}else if(receivedType == C_END){

				}
				else{
					printf("Unknown packet received\n");
				}

			} while(receivedType != C_END);
			
			close(fd);
			if(!failure){
				llclose();
				if(extractFileInfo(applicationLayerAttrs.buffer, &filename2, &filesize2)){
					printf("Error in last packet\n");
					failure = 1;
				}else{
					if(filesize2 != filesize || filesize != numReceivedBytes || filesize2 != numReceivedBytes){
						printf("File size mismatch: N1: %d, N2: %d, Received: %d\n", filesize, filesize2, numReceivedBytes);
						failure = 1;
					}else printf("Received %d bytes\n", numReceivedBytes);
					if(strcmp(filename, filename2) != 0){
						printf("File name mismatch: F1: %s, F2: %s\n", filename, filename2);
						failure = 1;
					}
					printf("Received Last Packet\n");

				}
			}
		}
	}
	if(filename != NULL)
		free(filename);
	if(filename2 != NULL)
		free(filename2);

	return -failure;
}

void printApplicationLayerStats(){
	printLinkLayerStats();
}

