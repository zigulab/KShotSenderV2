#pragma once

// sendManager ��� ����Դϴ�.


#include "CommManager.h"
#include "ClientManager.h"
#include "DBManager.h"

// �߼�ť ����
#include "SendQueueInfo.h"


// ��� ����
struct RESULT_ITEM
{
	char uid[20];
	char send_number[20];
	int result;
	char receiveNo[20];
	char done_time[20];
	int telecom;
	unsigned int kshot_number;
	time_t	regist_time;	
};

class SendManager : public CObject
{
public:
	SendManager();
	virtual ~SendManager();

/////////////////////////////////////////////////////////////////////////// DATA

	// ��Ű���
	CommManager					_commManager;

	// ������
	ClientManager				_clientManager;

	// DB����
	DBManager					_dbManager;
	
	// �߼� ť
	vector<SEND_QUEUE_INFO*>	_sendQueue;

	// ��� ť
	vector<RESULT_ITEM*>		_msgresult;

	// Ű �ʱⰪ(������)
	unsigned int				_keyinitvalue;

	// ����ȭ ��ü
	CCriticalSection			_cs;

	HANDLE						hResultDoneEvent;

	volatile BOOL				_bExitFlag;
	volatile BOOL				_bDoneThreadFlag;
	volatile BOOL				_bDoneResultFlag;

/////////////////////////////////////////////////////////////////////////// METHOD
	BOOL prevInit();

	BOOL	Init();

	void SetResultDoneEvent();

	BOOL Connect();

	BOOL Reconnect(TELECOM_SERVER_NAME server);

	void	UnInit();

	// �߼�
	void	SendProcess(CString id, int msgtype, char *send_number, char* callback, char* receive, char* subject, 
						char* msg, char* filename, char* filepath, int filesize, char isReserved, char* reservedTime, char* szRegTime);

	// ť�� ����
	bool	PopQueue(MYSQL* mysql, vector<SEND_QUEUE_INFO*>& queue);

	int		PopQueue(vector<SEND_QUEUE_INFO*>& queue);

	// ť�� �ʱ�ȭ
	void	flushQueue();
	void	flushQueue(vector<SEND_QUEUE_INFO*>& queue);

	// Ű ����(������)
	int		GenerateKshotNumber();

	// kshotnumber�� ���� ���ť�� ��� ����
	CString FindUidFromKshotNumber(unsigned int kshot_number);
	CString FindSendNumberFromKshotNumber(unsigned int kshot_number);
	RESULT_ITEM* FindResultKshotNumber(unsigned int kshot_number);

	// �߼�ť item ����
	//SEND_ITEM* MakeSendItem(CString id, int msgtype, char *send_number, unsigned int kshot_number, char* callback, 
	//								char* receive, char* subject, char* msg, char* filename, char* filepath, 
	//								int filesize, char isReserved, char* reservedTime);

	// ���� ���� üũ
	bool	IsExpireReservedTime(SEND_QUEUE_INFO* ginfo);
	
	// ������������ üũ
	bool	CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime);
	

	// ���ť�� �߰�
	bool	AddResultQueue(SEND_QUEUE_INFO* info);

	// ���ť�� �߰�
	void	AddResultItem(RESULT_ITEM * item);

	// ���ť item����
	RESULT_ITEM*	MakeResultItem(CString uid, char* send_number, unsigned int kshot_number);

	// ��û�� ���� ����
	void	ResponseFromTelecom(char *uid_send_number, int result, char* time);

	// ��� ���
	void	PushResult(char* kshot_number, int result, char* time, int telecom);

	// ��� ����( client���� ȣ�� )
	bool	GetResult(CString id, char* send_number, int *result, char* time, int* telecom);

	// ���� �ð����
	time_t	GetNowTime();

	// �̼��Ű�� ���ῡ ���� ���� ó��
	void	FailForExpireItem();

	static UINT ResultDoneProc(LPVOID lpParameter);
};


