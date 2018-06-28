#pragma once

// CommManager ��� ����Դϴ�.

#include "CommKT.h"

// �������� ����
typedef enum
{
	SMS_TYPE_SMS,
	SMS_TYPE_LMS,
	SMS_TYPE_MMS
}SMS_TYPE;

// �������� ����
typedef enum
{
	SMS_SUBTYPE_NORMAL,
	SMS_SUBTYPE_DONGBO,
	SMS_SUBTYPE_MAX
}SMS_SUBTYPE;


// ��� ���� ����
typedef enum 
{
	LINE_KT,
	LINE_SKT,
	LINE_LG
} TELECOM_LINE;

class CommManager : public CObject
{
public:
	CommManager();
	virtual ~CommManager();

///////////////////////////////////////////////////////////////////////////////// MEMBER DATA

	TELECOM_LINE	_tele_line;

	CommBase*		_messenger;

///////////////////////////////////////////////////////////////////////////////// METHOD
	virtual BOOL	Init();

	virtual BOOL	ConnectToTelecom();
	virtual BOOL	ConnectToXroShot();
	virtual BOOL	ConnectToIntelli();

	virtual BOOL	Uninit();

	virtual void	Ping(int tid);

	virtual BOOL	SendSMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendLMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg);
	virtual BOOL	SendMMS(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);
	virtual BOOL	SendFAX(CString sendId, CString callbackNO, CString receiveNO, CString subject, CString msg, CString filename, CString imageFilePath, int size);

	virtual	BOOL	Receive(byte *msg, int len);

// GENERAL
};


