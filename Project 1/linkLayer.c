include "linkLayer.h"

static void fillNonData(unsigned char* frame, unsigned char a, unsigned char c){
	frame[0] = FLAG;
	frame[1] = a;
	frame[2] = c;
	frame[3] = a^c;
	frame[4] = FLAG;
}

static void buildNonData(unsigned char* frame, int* size, int role, int type, int num){
	unsigned char c;
	if(type == C_RR_DEFAULT)
		c = C_RR(num);
	else if(type == C_REJ_DEFAULT)
		c = C_REJ(num);
	else if(type == C_I_DEFAULT)
		c= C_I(num);
	else c = type;
	unsigned char a = A(role, c);
	fillNonData(frame, a, c);
	*size = 5;
}

static void stuff(unsigned char* frame, int* size){
	int i;
	for(i = 1; i < *size-1; i++){
		/*printf("iteration %d , size %d: ", i, *size);
		for(j = 0; j < *size; j++) {
		printf("0x%x ", frame[j]);
		}
		printf("\n\n");*/
		if(frame[i] == REPLACEABLE1){
			memmove((void * )(frame + i + 2), (void *)(frame + i + 1), (size_t)(*size-i-1));
			frame[i] = REPLACED;
			frame[i+1] = REPLACED1;
			i++;
			(*size)++;
		}
		else if(frame[i]== REPLACEABLE2){
			memmove((void * )(frame + i + 2), (void *)(frame + i + 1), (size_t)(*size-i-1));
			frame[i] = REPLACED;
			frame[i+1] = REPLACED2;
			i++;
			(*size)++;
		}
	}
	
}




static void fillData(unsigned char* frame, unsigned char a, unsigned char c, unsigned char* data, int* numtoWrite){
	frame[0] = FLAG;
	frame[1] = a;
	frame[2] = c;
	frame[3] = a^c;
	memcpy((void *)(frame + 4), (void *)data, *numtoWrite);
	unsigned char bcc = 0;
	int i = 0;
	for(i = 4; i < *numtoWrite + 4; i++){
		bcc ^= frame[i];
	}
	frame[*numtoWrite + 4] = bcc;
	frame[*numtoWrite + 5] = FLAG;
	*numtoWrite += 6;
	stuff(frame, numtoWrite);
}
/*static void printFrame( unsigned char* buffer, int num){
	int i = 0;
	for(; i < num; i++){
		printf("0x%02x ", buffer[i]);
	}
}*/

static void buildData(unsigned char* frame, unsigned char* data, int* numtoWrite, int role, int num){
	int c = C_I(num);
	int a = A(role, c);
	fillData(frame, a, c, data, numtoWrite);
}

//to be applied to an entire frame, delimited by FLAGs, after destuffing
static int processReceivedFrame2(unsigned char * frame, int* numReceived, int role, int number){
	int i;
	if(*numReceived < 5){
		printf("Not enough bytes\n");
		return 0;
	}
	if(frame[0] != FLAG || frame[*numReceived - 1] != FLAG){
		printf("Frame Not Delimited\n");
		return 0;
	}
	if(frame[3] != (frame[1] ^ frame[2])){
		printf("BCC1 Wrong\n");
		return 0;
	}
	if(frame[1] == A_1){
		if(role){
			if(!IS_CMD(frame[2])){
				printf("As transmitter, expected Command\n");
				return 0;
			}
		}else {
			if(!IS_REPLY(frame[2])){
				printf("As receiver, expected Reply\n");
				return 0;
			}
		}

	}
	else if(frame[1] == A_2){
		if(role){
			if(!IS_REPLY(frame[2])){
				printf("As transmitter, expected Reply\n");
				return 0;
			}
		}else {
			if(!IS_CMD(frame[2])){
				printf("As receiver, expected Command\n");
				return 0;
			}
		}
	}
	else {
		printf("Invalid A field\n");
		return 0;
	}
	/*if(IS_NUMBERED(frame[2])){
		if(IS_C_RR(frame[2])){
			if((!number) != EXTRACT_SEQUENCE_NR(frame[2])){
				printf("Wrong sequence number in REJ\n");
				return 0;
			}
		}else if(number != EXTRACT_SEQUENCE_NR(frame[2])){
			printf("Wrong sequence number\n");
				return 0;
		}
	}*/
	///0x21
	// 0010 0001
	if(IS_C_RR(frame[2])){
		if(number == EXTRACT_SEQUENCE_NR(frame[2])){
				printf("Wrong sequence number in RR, is %d, expected %d\n", EXTRACT_SEQUENCE_NR(frame[2]), !number);
				return 0;
			}
	} else if (IS_C_REJ(frame[2])){
			if((!number) == EXTRACT_SEQUENCE_NR(frame[2])){
				printf("Wrong sequence number in REJ\n");
				return 0;
			}
	}
	if(!IS_C_I(frame[2])){
		if(*numReceived != 5){
			printf("Non Data has data\n");
			return 0;
		}
	}
	else{
		unsigned char bcc2 = 0;
		for(i = 4; i < *numReceived - 2; i++){
			//printf("xor:%x\n", bcc2);
			bcc2 ^=frame[i];
		}
		//printf("xorf:%x\n", bcc2);
		if(bcc2 != frame[*numReceived - 2]){
			printf("BCC2 Wrong, expected 0x%x found 0x%x\n", bcc2, frame[*numReceived - 2]);
			return 0;
		}
	}
	return 1;

}

static void destuff(unsigned char* frame, int* size){
	int i;
	for(i = 0; i < *size+1; i++){
		if(frame[i] == REPLACED){
			unsigned char value =0;
			if(frame[i+1] == REPLACED1)
				value = REPLACEABLE1;
			else if(frame[i+1] == REPLACED2)
				value = REPLACEABLE2;
			if(value != 0){
			memmove((void * )(frame + i+1), (void *)(frame + i + 2),(size_t) *size);
			frame[i] = value;
			(*size)--;
			}
		}
	}
	
}

static struct termios oldtio;
static struct linkLayer linkLayerAttrs;
static struct linkLayerStats stats;

static int openPort(int port){
    //int fd,c, res;
    struct termios newtio;
   
   // int i, sum = 0, speed = 0;
    
    sprintf(linkLayerAttrs.port, "/dev/ttyS%d", port);
    
    linkLayerAttrs.fd = open(linkLayerAttrs.port, O_RDWR | O_NOCTTY );
    printf("Opened Port %s\n", linkLayerAttrs.port);
    if (linkLayerAttrs.fd < 0) {perror(linkLayerAttrs.port); return -1; }

    if ( tcgetattr(linkLayerAttrs.fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = linkLayerAttrs.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = OPOST;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(linkLayerAttrs.fd, TCIFLUSH);

    if ( tcsetattr(linkLayerAttrs.fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    return linkLayerAttrs.fd;
}

static int closePort(){
    if ( tcsetattr(linkLayerAttrs.fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      return -1;
    }
    return close(linkLayerAttrs.fd);
}


static void initLinkLayerStats(struct linkLayerStats* stat){
	stat->informationFrameNumber = 0;
	stat->timeoutNumber = 0;
	stat->rejNumber = 0;
	stat->framesDiscarded = 0;
}
//return fd
int initLinkLayer(int port, int baudrate, int timeout, int numTransmissions, int role, int segmentSize, float errorProbability){
	linkLayerAttrs.baudRate = baudrate;
	linkLayerAttrs.timeout = timeout;
	linkLayerAttrs.numTransmissions = numTransmissions;
	linkLayerAttrs.receptionSize = 0;
	linkLayerAttrs.transmissionSize = 0;
	linkLayerAttrs.role = role;
	linkLayerAttrs.giveUp = 0;
	linkLayerAttrs.sequenceNumber= 0;
	linkLayerAttrs.receptionFrame = (unsigned char*) malloc(FRAME_SIZE(segmentSize)*sizeof(unsigned char));
	linkLayerAttrs.transmissionFrame = (unsigned char*) malloc(FRAME_SIZE(segmentSize)*sizeof(unsigned char));
	linkLayerAttrs.maxSize = FRAME_SIZE(segmentSize);
	linkLayerAttrs.errorProbability = errorProbability;
	initLinkLayerStats(&stats);
	openPort(port);
	return linkLayerAttrs.fd;
}

int closeLinkLayer(){
	if(linkLayerAttrs.receptionFrame != NULL){
		free(linkLayerAttrs.receptionFrame);
		linkLayerAttrs.receptionFrame = NULL;
	}
	if(linkLayerAttrs.transmissionFrame != NULL){
		free(linkLayerAttrs.transmissionFrame);
		linkLayerAttrs.transmissionFrame = NULL;
	}
	return closePort();
}

void printLinkLayerStats(){
	printf("Stats:\n");
	printf("\tNumber of Information Frames: %d\n", stats.informationFrameNumber);
	printf("\tNumber of Timeouts: %d\n", stats.timeoutNumber);
	printf("\tNumber of REJ frames: %d\n", stats.rejNumber);
	printf("\tNumber of frames discarded: %d\n", stats.framesDiscarded);
}

static void randomizeByte(unsigned char * byte){
	//if(linkLayerAttrs.role) return ;
	
	if(((float)rand())/UINT_MAX < linkLayerAttrs.errorProbability){
		unsigned char other;
		do{
			other = (unsigned char) rand();
		}while(other == *byte);
		*byte = other;
	}
}

static int receiveOnce(int fd, unsigned char* buffer, int startWithFlag, int timeout, int max){
	//printf("Receiving...\n");
	int i = 0;
	if(startWithFlag){
		buffer[0] = FLAG;
		i = 1;
	}
	unsigned char byte;
	int numRead;
	do{
		alarm(timeout);
		if(i > max)
			return -2;
		numRead = read(fd, &byte, 1);
		randomizeByte(&byte);
		if(!linkLayerAttrs.role){
			//printf("0x%02x ", byte);
		}
		if(linkLayerAttrs.giveUp){
			printf("receiveOnce() giving up :(\n");
			return -1;
		}
		if(numRead <= 0){
			//printf("receiveOnce() read 0 bytes\n");
			continue;
		}
		if(i == 0 && byte != FLAG){
			//printf("receiveOnce() read NON FLAG at i == 0, 0x%02x\n", byte);
			continue;
		}
		if(i == 1 && byte == FLAG)
			continue;
		//printf("receiveOnce() read 0x%02x at i == %d\n", byte, i);
		buffer[i] = byte;
		i++;
		//printFrame(buffer, i);
		//printf("\n");
	}while(!(byte == FLAG && i > 1));
	//printf("Received : \n");
	//printFrame(buffer, i);
	//printf("\n");

	destuff(buffer, &i);
	if(!linkLayerAttrs.role){
	//printf("\n");
	}
	//printf("Destuffed : ");
	//printFrame(buffer, i);
	//printf("\n");

	return i;
}



static int verifyFrameType(unsigned char* buffer, int type){
	unsigned char utype = (unsigned char) type;
	unsigned char frameType = buffer[2];
	switch(utype){
		case C_SET: return frameType == C_SET;
		break;
		case C_DISC: return frameType == C_DISC;
		break;
		case C_UA: return frameType == C_UA;
		break;
		case C_RR_DEFAULT: return IS_C_RR(frameType);
		break;
		case C_REJ_DEFAULT: return IS_C_REJ(frameType);
		break;
		case C_I_DEFAULT: return IS_C_I(frameType);
		break;
	}
	return 0;
}

static int getFrameNumber(unsigned char* buffer){
	if(IS_NUMBERED(buffer[2]))
	{
		return EXTRACT_SEQUENCE_NR(buffer[2]);
	}else{
		return -1;
	}
}


static void transmit(){
	//unsigned char flag = FLAG;
	//write(linkLayerAttrs.fd, &flag, 1);

	int res = write(linkLayerAttrs.fd, linkLayerAttrs.transmissionFrame, linkLayerAttrs.transmissionSize);
	if(res != linkLayerAttrs.transmissionSize)
		printf("Could not transmit\n");
	//printf("Transmitting ");
	//printFrame(linkLayerAttrs.transmissionFrame, linkLayerAttrs.transmissionSize);
	//printf("\n");
}

static int receive(int sendRej, int count, ...){
	
	int sucess = 0;
	va_list argp;
	va_start(argp, count);
	int type;
	int startWithFlag = 0;
	while(!linkLayerAttrs.giveUp){
		
		int numReceived = receiveOnce(linkLayerAttrs.fd, linkLayerAttrs.receptionFrame, startWithFlag, linkLayerAttrs.timeout, linkLayerAttrs.maxSize);
		if(numReceived < 0){
			//printf("receiveOnce returned negative\n");
			continue;
		}
		startWithFlag = 1;
		linkLayerAttrs.receptionSize = numReceived;
		if(processReceivedFrame2(linkLayerAttrs.receptionFrame, &linkLayerAttrs.receptionSize, !linkLayerAttrs.role, linkLayerAttrs.sequenceNumber)){
			int i = 0;
			for(i = 0; i < count ; i++){
				type = va_arg(argp, int);
				if(verifyFrameType(linkLayerAttrs.receptionFrame, type)){
					//printf("Received Frame of Type 0x%x\n", type);
					sucess = 1;
					linkLayerAttrs.attempts = 0;
					startWithFlag = 0;
					goto endloop;
				}//else printf("Invalid Frame Type Received, expected 0x%x, but was 0x%x\n", type, linkLayerAttrs.receptionFrame[2]);
				
			}
		} else {
			printf("Invalid Frame Received ");
			stats.framesDiscarded++;
			//printFrame(linkLayerAttrs.receptionFrame, linkLayerAttrs.receptionSize);
			//printf("\n");
		}
		if(!sucess && sendRej){
			buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_REJ_DEFAULT, linkLayerAttrs.sequenceNumber);
			transmit();
			stats.rejNumber++;
		}
	}
	
	endloop:
	va_end(argp);
	//alarm(0);
	return sucess;
}


static void transmitterOnTimeout(int sig){
	stats.timeoutNumber++;
	if(IS_C_I(linkLayerAttrs.transmissionFrame[2])){
		stats.informationFrameNumber++;
	}
	if(linkLayerAttrs.attempts >= linkLayerAttrs.numTransmissions-1){alarm(0); linkLayerAttrs.giveUp = 1; printf("Max Attempts reached\n");}
	else {
		printf("Retransmitting\n");
		//printf("Transmitting ");
		//printFrame(linkLayerAttrs.transmissionFrame, linkLayerAttrs.transmissionSize);
		//printf("\n");
		transmit();
		linkLayerAttrs.attempts++;
		alarm(linkLayerAttrs.timeout);
	}	
}
static void receiverOnTimeout(int sig){
	if(linkLayerAttrs.attempts >= linkLayerAttrs.numTransmissions-1){alarm(0); linkLayerAttrs.giveUp = 1; printf("Max Attempts reached\n");}
	else {
		printf("Reconnecting\n");
		linkLayerAttrs.attempts++;
		alarm(linkLayerAttrs.timeout);
	}
}

static int installAlarmHandler(void (*handler)(int), int timeout){
	struct sigaction action1;
	sigaction(SIGALRM,NULL, &action1);
	action1.sa_handler = handler;
	if (sigaction(SIGALRM,&action1,NULL) < 0)
	{
		fprintf(stderr,"Unable to install SIGALRM handler\n");
		return 0;
	}
	alarm(timeout);
	return 1;
}

int llopen(){
	
	linkLayerAttrs.giveUp = 0;
	int sucess = 0;
	if(linkLayerAttrs.role){
		installAlarmHandler(transmitterOnTimeout, linkLayerAttrs.timeout);
		buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_SET, 0);
		transmit();
		if(receive(0, 1, C_UA))
			sucess = 1;
	}
	else{
		installAlarmHandler(receiverOnTimeout, linkLayerAttrs.timeout*3);
		if(receive(0, 1, C_SET)){
			buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_UA, 0);
			transmit();
			sucess = 1;
		}
	}
	linkLayerAttrs.attempts = 0;
	alarm(0);
	return sucess;
}

int llwrite(unsigned char* buffer, int length){
	linkLayerAttrs.giveUp = 0;
	int sucess = 0;
	if(linkLayerAttrs.role){
		installAlarmHandler(transmitterOnTimeout, linkLayerAttrs.timeout);
		linkLayerAttrs.transmissionSize = length;
		buildData(linkLayerAttrs.transmissionFrame, buffer, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, linkLayerAttrs.sequenceNumber);
		transmit();
		stats.informationFrameNumber++;
		int repeat = 1;
		while(repeat){
			if(receive(0, 2, C_RR_DEFAULT, C_REJ_DEFAULT)){
				//printf("llwrite() received a frame\n");
				if(verifyFrameType(linkLayerAttrs.receptionFrame, C_REJ_DEFAULT)){
					alarm(linkLayerAttrs.timeout);
					transmit();
					stats.rejNumber++;
					//printf("llwrite() received REJ\n");
					//printFrame(linkLayerAttrs.transmissionFrame, linkLayerAttrs.transmissionSize);
					//printf("\n");
					//exit(1);
				}
				else if(verifyFrameType(linkLayerAttrs.receptionFrame, C_RR_DEFAULT)){
					linkLayerAttrs.sequenceNumber ^= 1;
					sucess = 1;
					repeat = 0;
					//printf("llwrite() received RR\n");
				}else{
					printf("---------->Control should not reach here!!<-------------");
				}
			}else {
				repeat = 0;
			}
		}
	}
	alarm(0);
	linkLayerAttrs.attempts = 0;
	if(sucess)
		return length;
	else {
		printf("llwrite failed\n");
		return 0;
	}
}

int llread(unsigned char* buffer){
	linkLayerAttrs.giveUp = 0;
	int sucess = 0;
	if(!linkLayerAttrs.role){
		installAlarmHandler(receiverOnTimeout, linkLayerAttrs.timeout);
		int repeat = 1;
		while(repeat){
			if(receive(1, 2, C_I_DEFAULT, C_SET)){
				if(verifyFrameType(linkLayerAttrs.receptionFrame, C_SET)){
					buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_UA, 0);
					transmit();
				}else if(verifyFrameType(linkLayerAttrs.receptionFrame, C_I_DEFAULT)){
					stats.informationFrameNumber++;
					if(getFrameNumber(linkLayerAttrs.receptionFrame) == linkLayerAttrs.sequenceNumber){
						linkLayerAttrs.sequenceNumber ^= 1;
						sucess = 1;
						repeat = 0;
						
						//printf("Received next frame\n");
					}else {
						printf("Received previous frame\n");
					}
					buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_RR_DEFAULT, linkLayerAttrs.sequenceNumber);
					transmit();	
				}else {
					printf("---------->Control should not reach here!!<-------------");
				}
			} else{
				repeat = 0; // timeout
			}
		}
	}
	alarm(0);
	linkLayerAttrs.attempts = 0;
	if(sucess){
		memcpy((void*)buffer, (void *)linkLayerAttrs.receptionFrame + 4, linkLayerAttrs.receptionSize - 6);
		
		return linkLayerAttrs.receptionSize - 6;
	}
	
	else {
		printf("llread failed\n");
		return -1;
	}
}

int llclose(){
	linkLayerAttrs.giveUp = 0;
	int sucess = 0;
	if(linkLayerAttrs.role){
		installAlarmHandler(transmitterOnTimeout, linkLayerAttrs.timeout);
		buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_DISC, 0);
		transmit();
		if(receive(0, 1, C_DISC)){
			buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_UA, 0);
			transmit();
			sucess = 1;
		}
	}
	else{
		installAlarmHandler(receiverOnTimeout, linkLayerAttrs.timeout*3);
		int receivedDisc = 0;
		int repeat = 1;
		while(repeat){
			if(receive(0, 3, C_DISC, C_UA, C_I_DEFAULT)){
				//printf("Received Somethin\n");
				if(verifyFrameType(linkLayerAttrs.receptionFrame, C_UA)){
					//printf("Received UA\n");
					if(receivedDisc){
						sucess = 1;
						repeat = 0;
					}
				}
				else if (verifyFrameType(linkLayerAttrs.receptionFrame, C_DISC)){
					//printf("Received Disc\n");
					receivedDisc = 1;
					buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_DISC, 0);
					transmit();
				}
				else if (verifyFrameType(linkLayerAttrs.receptionFrame, C_I_DEFAULT)){
					//printf("Received C_I in llclose()\n");
					buildNonData(linkLayerAttrs.transmissionFrame, &linkLayerAttrs.transmissionSize, linkLayerAttrs.role, C_RR_DEFAULT, !getFrameNumber(linkLayerAttrs.receptionFrame));
					transmit();
				}
				else {
					printf("---------->Control should not reach here!!<-------------");
				}
			} else{
				printf("Reception Failed\n");
				repeat = 0; //timeout
			}
		}
	}
	alarm(0);
	linkLayerAttrs.attempts = 0;
	return sucess;
}