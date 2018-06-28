#pragma once

// ClientManager 명령 대상입니다.

#include "ClientThreadPool.h"
//#include "ListenSocket.h"

#include "SendResultInfo.h"


class ClientAcceptor;

class Client;
class ClientManager : public CObject
{
public:
	ClientManager();
	virtual ~ClientManager();

/////////////////////////////////////////////////////////////////////////// DATA

	// 클라이언트 생성 풀
	ClientThreadPool			_pool;

	// 연결 접수기
	ClientAcceptor*			_acceptor;
		
	// 포트
	int								_port;

	// 문자 전송요청 없이 제한시간(1800초[30분])이상 경과됨.연결 종료 처리
	BOOL							_close_idleclient;

	// 미수신 만료 처리 시간
	int								_expire_time;

	// 결과 저장 
	struct SEND_RESULT_INFO
	{
		Client* client;
		CString send_number;
		unsigned int kshot_number;
		CString receiveNo;
		CString done_time;
		int result; 
		int telecom;
		int msgType;
		float price;
		CString pid;

		// KT로 부터 결과 수신
		BOOL doneflag;

		// KT로 부터 결과 수신 시간
		DWORD result_time;

		// Agent 결과 송신 완료
		BOOL completeflag;

		// 등록(발송)시간
		CTime reg_time;
	};

	HANDLE						_hExitEvent;

	BOOL							_bDoneThreadProc[2];

	// 사용않함.
	vector<SEND_RESULT_INFO*>	_vtSendResult;

	// uidnumber와 SEND_RESULT_INFO 연관
	map<CString, SEND_RESULT_INFO*>	_mapSendResult;

	// kshot_number와 uidnumber 연관
	map<unsigned int, CString>	_mapKShotKey_UidSendNumber;

	// 전송 결과 파일, 서버 종료시 저장, 시작시 로드
	CString						_SEND_RESULT_FILE;

	// 이전 전송 결과 저장
	CSendResultInfo			_prevSendResultInfo;

	CCriticalSection			_cs;

	CCriticalSection			_cs2;

	map<CString, Client*> _mapClients;

/////////////////////////////////////////////////////////////////////////// METHOD

	virtual		bool	Init();
	virtual		bool	UnInit();

	virtual		void	OnAccept();

	int GetClientCount();
	CString GetClientId(int index);

	void AddClient(CString uid, Client* client);
	void DelClient(CString uid, Client* client);
	Client* GetClient(CString uid);

	void UpdateClientList(CString uid, CString lastRequestDate, int lastRequestCount);

	// 결과 알림 재시도
	void RetryResultNotify(Client* client, CString uid);

	int GetResult(map<CString, SEND_RESULT_INFO*>& result);

	int GetDoneResult(map<CString, SEND_RESULT_INFO*>& result);

	void AddResult(Client *client, unsigned int kshot_number, CString send_number, CString receiveNo, int msgType, float priceValue, CString pid);

	void GetId_Price_MsgType(CString uid_sendno, unsigned int kshot_number, CString& uid, CString& pid, float* price, int* msgType);

	void SetResultKShotNumber(CString uid, CString send_number, unsigned int kshot_number);

	CString GetUidSendNumberByKShotKey(unsigned int kshot_number);

	void SetResult(CString uid_sendno, unsigned int kshot_number, int result, CString done_time, int telecom);

	void DelResult(SEND_RESULT_INFO *info);
	
	void FlushResult();

	void FlushResult(map<CString, SEND_RESULT_INFO*>& doneResult);

	HANDLE GetExitEvent();

	void SetDoneThread(int index);

	static UINT ResultProc(LPVOID lpParameter);
	static UINT LinkCheckProc(LPVOID lpParameter);

	unsigned int GetKShotSendNumber();
};


