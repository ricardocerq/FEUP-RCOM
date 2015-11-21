#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)<(b))?(b):(a))

#define BAUDRATE B38400
#define BAUDRATE_300 B300
#define BAUDRATE_600 B600
#define BAUDRATE_1200 B1200
#define BAUDRATE_2400 B2400
#define BAUDRATE_4800 B4800
#define BAUDRATE_9600 B9600
#define BAUDRATE_19200 B19200
#define BAUDRATE_38400 B38400
#define BAUDRATE_57600 B57600
#define BAUDRATE_115200 B115200


// SET : [FLAG|A|C|BCC|FLAG]
#define FLAG 0x7E

//BCC A^C
#define REPLACEABLE1 0x7e
#define REPLACEABLE2 0x7d
#define REPLACED 0x7d
#define REPLACED1 0x5e
#define REPLACED2 0x5d
#define TRANSMITTER 1
#define RECEIVER 0

#define MAX_SIZE 1024

#define PACKET_MAX_SIZE (MAX_SIZE + 4)
#define FRAME_MAX_SIZE (PACKET_MAX_SIZE * 2 + 7)

#define PACKET_SIZE(SIZE) (SIZE + 8)
#define FRAME_SIZE(SIZE) (PACKET_SIZE(SIZE) * 2 + 7)


#define C_SET 0x07 //Command
#define C_DISC 0x0B //Command
#define C_UA 0x03 //Reply
#define C_RR_DEFAULT 0x01
#define C_RR(N) ((N << 5) | C_RR_DEFAULT) //Reply
#define IS_C_RR(C) (C == C_RR(0) || C == C_RR(1))
#define C_REJ_DEFAULT 0x05
#define C_REJ(N) ((N << 5) | C_REJ_DEFAULT) //Reply
#define IS_C_REJ(C) (C == C_REJ(0) || C == C_REJ(1))
#define C_I_DEFAULT 0x00
#define C_I(N) ((N << 5)|C_I_DEFAULT) //Command
#define IS_REPLY(C) (C==C_UA || C==C_RR(0) || C==C_RR(1) || C==C_REJ(0) ||C==C_REJ(1))
#define IS_CMD(C) (C==C_SET || C==C_DISC || C==C_I(0) ||C==C_I(1))
#define IS_C_I(C) (C==C_I(0)|| C == C_I(1))
#define IS_NUMBERED(C)(IS_C_RR(C) || IS_C_REJ(C) || IS_C_I(C))
#define EXTRACT_SEQUENCE_NR(C) (((1<<5)&C) >> 5)


#define A(role, C) (IS_REPLY(C)? (role ? 0x01 : 0x03) : (role ? 0x03 : 0x01))
                        			 //true if transmitter
    	                      
		             
#define IS_A(field) (field == 0x03 || field == 0x01)
#define A_1 0x03
#define A_2 0x01
#define BCC1(a, c) (a^c)

#define C_DATA 0
#define C_START 1
#define C_END 2
#define T_SIZE 0
#define T_NAME 1


#endif