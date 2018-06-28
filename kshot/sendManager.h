#pragma once

// sendManager 명령 대상입니다.


#include "CommManager.h"
#include "ClientManager.h"
#include "DBManager.h"

// 발송큐 정보
#include "SendQueueInfo.h"


// 결과 정보
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

	// 통신관리
	CommManager					_commManager;

	// 고객관리
	ClientManager				_clientManager;

	// DB관리
	DBManager					_dbManager;
	
	// 발송 큐
	vector<SEND_QUEUE_INFO*>	_sendQueue;

	// 결과 큐
	vector<RESULT_ITEM*>		_msgresult;

	// 키 초기값(사용안함)
	unsigned int				_keyinitvalue;

	// 동기화 객체
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

	// 발송
	void	SendProcess(CString id, int msgtype, char *send_number, char* callback, char* receive, char* subject, 
						char* msg, char* filename, char* filepath, int filesize, char isReserved, char* reservedTime, char* szRegTime);

	// 큐에 빼기
	bool	PopQueue(MYSQL* mysql, vector<SEND_QUEUE_INFO*>& queue);

	int		PopQueue(vector<SEND_QUEUE_INFO*>& queue);

	// 큐에 초기화
	void	flushQueue();
	void	flushQueue(vector<SEND_QUEUE_INFO*>& queue);

	// 키 생성(사용안함)
	int		GenerateKshotNumber();

	// kshotnumber를 통해 결과큐의 결과 추출
	CString FindUidFromKshotNumber(unsigned int kshot_number);
	CString FindSendNumberFromKshotNumber(unsigned int kshot_number);
	RESULT_ITEM* FindResultKshotNumber(unsigned int kshot_number);

	// 발송큐 item 생성
	//SEND_ITEM* MakeSendItem(CString id, int msgtype, char *send_number, unsigned int kshot_number, char* callback, 
	//								char* receive, char* subject, char* msg, char* filename, char* filepath, 
	//								int filesize, char isReserved, char* reservedTime);

	// 예약 만기 체크
	bool	IsExpireReservedTime(SEND_QUEUE_INFO* ginfo);
	
	// 예약일자형식 체크
	bool	CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime);
	

	// 결과큐에 추가
	bool	AddResultQueue(SEND_QUEUE_INFO* info);

	// 결과큐에 추가
	void	AddResultItem(RESULT_ITEM * item);

	// 결과큐 item생성
	RESULT_ITEM*	MakeResultItem(CString uid, char* send_number, unsigned int kshot_number);

	// 요청에 대한 응답
	void	ResponseFromTelecom(char *uid_send_number, int result, char* time);

	// 결과 등록
	void	PushResult(char* kshot_number, int result, char* time, int telecom);

	// 결과 얻어가기( client에서 호출 )
	bool	GetResult(CString id, char* send_number, int *result, char* time, int* telecom);

	// 현재 시간얻기
	time_t	GetNowTime();

	// 미수신결과 만료에 대한 실패 처리
	void	FailForExpireItem();

	static UINT ResultDoneProc(LPVOID lpParameter);
};


