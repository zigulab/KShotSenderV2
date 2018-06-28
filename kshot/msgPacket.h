
#pragma once

#pragma pack(push,1)  

//typedef unsigned char       INT8;
//typedef short int			INT16;
//typedef int					INT32;

#define 	CPT_PKT_LINKCHK			0x00000100
#define 	CPT_PKT_LINKCHK_ACK		0x10000100
#define 	CPT_PKT_GRANT			0x00000101
#define 	CPT_PKT_GRANT_ACK		0x10000101
#define 	CPT_PKT_BIND			0x00000102
#define 	CPT_PKT_BIND_ACK		0x10000102
#define 	CPT_PKT_MESSAGE			0x00000103
#define 	CPT_PKT_MESSAGE_ACK		0x10000103
#define 	CPT_PKT_REPORT			0x00000104
#define 	CPT_PKT_REPORT_ACK		0x10000104

typedef struct PKT_LINKCHK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
    INT16		RESERVED;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_LINKCHK;

typedef struct PKT_LINKCHK_ACK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
    INT16		RESERVED;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_LINKCHK_ACK;

typedef struct PKT_GRANT
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
    INT16		RESERVED;
	char		LOGIN_ID[16];
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_GRANT;

typedef struct PKT_GRANT_ACK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
    INT16		RESERVED;
	int			RESULT;
	char		BIND_ID[16];
	char		SERVER_IP[16];
	short int   PORT;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_GRANT_ACK;

typedef struct PKT_BIND
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
	char		LOGIN_ID[16];
	char		PWD[16];
	char		BIND_ID[16];
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_BIND;

typedef struct PKT_BIND_ACK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN; 
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
	int			RESULT;
	int			MAX_OFSEC;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_BIND_ACK;


typedef struct PKT_MESSAGE
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN;
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
	int			MSGTYPE;
	int			SEND_NUMBER;
	char		CALLBACK_NO[32];
	char		TARGET_NO[32];
	char		SUBJECT[100];
	char		MESSAGE[2000];
	char		FILENAME[100];
	char		UPLOADED_PATH[200];
	int			FILE_SIZE;
	char		IS_RESERVED;		// y, n
	char		RESERVED_TIME[30];
    INT8		PACKET_END0;
    INT8		PACKET_END1;
	int			TARGET_SIZE;
}PKT_MESSAGE;

typedef struct PKT_MESSAGE_ACK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN;
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
	int			RESULT;
	int			SEND_NUMBER;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_MESSAGE_ACK;

typedef struct PKT_REPORT
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN;
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
	int			RESULT;
	int			SEND_NUMBER;
	char		TARGET_NO[32];
	char		DONE_TIME[16];
	int			TELECOM_ID;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
}PKT_REPORT;


typedef struct PKT_REPORT_ACK
{
    INT8		PACKET_BEGIN0;
    INT8		PACKET_BEGIN1;
    INT32		MSGLEN;
    INT32		MSGID;
    INT32		REQUESTID;
    INT8		VERSION_MSB;
    INT8		VERSION_LSB;
	INT16		RESERVED;
    INT8		PACKET_END0;
    INT8		PACKET_END1;
	int			LIST_SIZE;
}PKT_REPORT_ACK;

#pragma pack(pop) 
