#pragma once

// HaomunAgent 명령 대상입니다.

#include "include/mysql/mysql.h"

// 발송큐 정보
#include "SendQueueInfo.h"

class DBManager;

class LGUPlusAgent : public CObject
{
public:
	LGUPlusAgent(DBManager*	dbManager);
	virtual ~LGUPlusAgent();

	////////////////////////////////////////////////////////////
	DBManager*	_dbManager;

	volatile BOOL	_bExitFlag;
	volatile BOOL	_bDoneThreadFlag;
	volatile BOOL	_bDoneThreadFlag2;

	HANDLE _hRunDelegatorEvent;
	HANDLE _hRunDelegatorEvent2;

	// 외부Agent에서 처리할 데이타
	vector<SEND_QUEUE_INFO*>	_vtOutAgentDatas;

	// 결과 저장
	struct resultPack {
		int     kshot_number;
		int		result;
		int		telecom;
		char	resultTime[30];
	};

	enum AGENT_STATE { AS_WAITING, AS_DOING };

	volatile AGENT_STATE _agent_state;

	// 처리중인 kshotNumber;
	vector<int>				_vtKshotNumber;

	map<int, int>			_mpKshotNumber;

	CString					_strKhostNumbers;

	CCriticalSection		_cs;
	CCriticalSection		_cs2;

	// 현재 일자의 결과 테이블명
	CString					_curResultTable;

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
	CString MakeJobList();
	void DeleteKshotNumber(vector<int>list);

	// 그 현재 row구한다.
	int GetResultRow(MYSQL& mysql, CString kshotNumberList);

	// sms 와 lms/mms 별도 테이블에서 row 얻기
	int GetResultRowWithTableName(MYSQL& mysql, CString kshotNumberList, CString tablename, BOOL isSMS);

	// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
	void GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString jobList);

	void GetResultWithTableName(MYSQL& mysql, vector<resultPack>& vtResultPack, CString jobList, CString tablename, BOOL isSMS);

	// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
	void SendAgentResult(vector<resultPack>& vtResultPack);

	// 결과 처리
	bool processResult(MYSQL& mysql, CString kshotNumberList);

	// 서버재시작후 kshot_number 설정
	bool SetKNumberWithReload(MYSQL& mysql);

	static UINT AgentProc(LPVOID lpParameter);
	static UINT ResultWatchProc(LPVOID lpParameter);
};


