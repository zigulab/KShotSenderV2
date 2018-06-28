#pragma once

// WiseCanAgent ��� ����Դϴ�.

#include "include/mysql/mysql.h"

// �߼�ť ����
#include "SendQueueInfo.h"

class DBManager;

class WiseCanAgent : public CObject
{
public:
	WiseCanAgent(DBManager*	dbManager);
	virtual ~WiseCanAgent();

	////////////////////////////////////////////////////////////
	DBManager*	_dbManager;

	volatile BOOL	_bExitFlag;
	volatile BOOL	_bDoneThreadFlag;
	volatile BOOL	_bDoneThreadFlag2;

	HANDLE _hRunDelegatorEvent;
	HANDLE _hRunDelegatorEvent2;

	// �ܺ�Agent���� ó���� ����Ÿ
	vector<SEND_QUEUE_INFO*>	_vtOutAgentDatas;

	CString	_curResultTable;

	// ��� ����
	struct resultPack {
		int		seq;
		int     kshot_number;
		int		result;
		char	telecom[20];
		char	resultTime[30];
	};

	enum AGENT_STATE {AS_WAITING, AS_DOING};

	volatile AGENT_STATE _agent_state;

	// ó������ kshotNumber;
	vector<int>				_vtKshotNumber;

	map<int, int>			_mpKshotNumber;

	CString					_strKhostNumbers;

	CCriticalSection		_cs;
	CCriticalSection		_cs2;

	////////////////////////////////////////////////////////////
	BOOL Init();
	BOOL UnInit();

	void	SetNewSendNotify();

	// ��û �б�/����/�����
	void			AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas);
	int				GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas);
	void			DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas);

	void SetKshotNumber(int number);
	CString MakeKshotNumberList();
	CString MakeJobList();
	void DeleteKshotNumber(vector<int>list);

	// �� ���� row���Ѵ�.
	int GetResultRow(MYSQL& mysql, CString kshotNumberList);

	// ���� ������ �ٸ��� ���ο� ����� �ִ°����� �Ǵ��ϰ� ����� �о���δ�.
	void GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString joblist);

	// ����� ���� msgResult�� �����ϰ� kshotAgent���� ����� ������.
	void SendAgentResult(vector<resultPack>& vtResultPack);

	// ��� ó��
	bool processResult(MYSQL& mysql, CString kshotNumberList);

	// ����������� kshot_number ����
	bool SetKNumberWithReload(MYSQL& mysql);

	static UINT AgentProc(LPVOID lpParameter);
	static UINT ResultWatchProc(LPVOID lpParameter);
};


