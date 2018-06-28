#pragma once

// FaxAgent 명령 대상입니다.


#include "include/mysql/mysql.h"

// 발송큐 정보
#include "SendQueueInfo.h"

class DBManager;

class FaxAgent : public CObject
{
public:
	FaxAgent(DBManager*	dbManager);
	virtual ~FaxAgent();

	////////////////////////////////////////////////////////////

	DBManager*	_dbManager;

	volatile BOOL	_bExitFlag;
	volatile BOOL	_bDoneThreadFlag;
	volatile BOOL	_bDoneThreadFlag2;

	HANDLE _hRunDelegatorEvent;
	HANDLE _hRunDelegatorEvent2;

	// 외부Agent에서 처리할 데이타
	vector<SEND_QUEUE_INFO*>	_vtOutAgentDatas;

	CString	_curResultTable;

	// 결과 저장
	struct resultPack {
		int		seq;
		int     kshot_number;
		int		result;
		//char	telecom[20];
		int telecom;
		char	resultTime[30];
	};

	enum AGENT_STATE { AS_WAITING, AS_DOING };

	volatile AGENT_STATE _agent_state;

	// 처리중인 kshotNumber;
	vector<int> _vtKshotNumber;

	CString		_strKhostNumbers;

	CCriticalSection _cs;

	////////////////////////////////////////////////////////////
	BOOL Init();
	BOOL UnInit();

	void	SetNewSendNotify();

	// 요청 읽기/쓰기/지우기
	void			AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas);
	int				GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas);
	void			DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas);

	void SetKshotNumber(int number);
	CString MakeKshotNumberList();
	void DeleteKshotNumber(vector<int>list);

	// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
	void GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString kshotNumberList);

	// 시스템 문제(쿼리, 기타)에 의한 일괄 실패처리
	void failProcessBySystemFault();

	// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
	void SendAgentResult(vector<resultPack>& vtResultPack);

	// 결과 처리
	BOOL processResult(MYSQL& mysql);

	int ConvertNRegTrans(MYSQL& mysql, CString faxFilePath, CString faxUserId, CString faxPwd, CString filepath, SEND_QUEUE_INFO* info);

	void SetStatusIntoSend(MYSQL& mysql, CString query, int loopcnt, int trans_seq);

	static UINT AgentProc(LPVOID lpParameter);
	static UINT ResultWatchProc(LPVOID lpParameter);
};


