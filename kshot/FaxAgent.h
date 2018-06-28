#pragma once

// FaxAgent ��� ����Դϴ�.


#include "include/mysql/mysql.h"

// �߼�ť ����
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

	// �ܺ�Agent���� ó���� ����Ÿ
	vector<SEND_QUEUE_INFO*>	_vtOutAgentDatas;

	CString	_curResultTable;

	// ��� ����
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

	// ó������ kshotNumber;
	vector<int> _vtKshotNumber;

	CString		_strKhostNumbers;

	CCriticalSection _cs;

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
	void DeleteKshotNumber(vector<int>list);

	// ���� ������ �ٸ��� ���ο� ����� �ִ°����� �Ǵ��ϰ� ����� �о���δ�.
	void GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString kshotNumberList);

	// �ý��� ����(����, ��Ÿ)�� ���� �ϰ� ����ó��
	void failProcessBySystemFault();

	// ����� ���� msgResult�� �����ϰ� kshotAgent���� ����� ������.
	void SendAgentResult(vector<resultPack>& vtResultPack);

	// ��� ó��
	BOOL processResult(MYSQL& mysql);

	int ConvertNRegTrans(MYSQL& mysql, CString faxFilePath, CString faxUserId, CString faxPwd, CString filepath, SEND_QUEUE_INFO* info);

	void SetStatusIntoSend(MYSQL& mysql, CString query, int loopcnt, int trans_seq);

	static UINT AgentProc(LPVOID lpParameter);
	static UINT ResultWatchProc(LPVOID lpParameter);
};


