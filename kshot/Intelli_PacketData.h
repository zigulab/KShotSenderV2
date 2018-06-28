
#ifndef __GROBAL_DEFINE__
#define __GROBAL_DEFINE__

#pragma pack(push,1)  

struct SMS_GRANT
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    char        LOGIN_ID[16];
    BYTE        Frame_End0;
    BYTE        Frame_End1;
 
};

struct SMS_GRANTACK
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         RESULT;
    char        BIND_ID[16];
    char        SERVER_IP[16];
    int         PORT;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
 
};

struct SMS_BIND
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    char        LOGIN_ID[16];
    char        LOGIN_PWD[16];
    char        BIND_ID[16];
    char        RV_MO_FLAG;
    char        RV_REPORT_FLAG;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
 
};

struct SMS_BINDACK
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         RESULT;
    int         TR_MAX_MPS;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
};

struct SMS_DELIVER
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         PID;
    int         SERIAL_NUM;
    char        mCALLBACK[32];
    char        DESTADDR[32];
    char        MESSAGE[120];
    BYTE        Frame_End0;
    BYTE        Frame_End1;
};

struct SMS_DELIVERACK  //26byte
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         RESULT;
    int         SERIAL_NUM;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
};

struct SMS_REPORT    //78byte
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         RESULT;
    int         SERIAL_NUM;
    char        DESTADDR[32];
    char        DELIVERY_TIME[16];
    int         TELECOM_ID;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
};

struct SMS_REPORTACK
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    int         RESULT;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
    
};

struct SMS_LINESEND
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
};

struct SMS_LINESENDACK
{
    BYTE        Frame_Start0;
    BYTE        Frame_Start1;
    short int   MSGLEN; 
    int         MSGID;
    int         Request_ID;
    BYTE        Version_MSB;
    BYTE        Version_LSB;
    short int   Filler;
    BYTE        Frame_End0;
    BYTE        Frame_End1;
        
};

#pragma pack(pop) 

#endif