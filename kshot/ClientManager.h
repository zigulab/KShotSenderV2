#pragma once

// ClientManager ��� ����Դϴ�.

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

	// Ŭ���̾�Ʈ ���� Ǯ
	ClientThreadPool			_pool;

	// ���� ������
	ClientAcceptor*			_acceptor;
		
	// ��Ʈ
	int								_port;

	// ���� ���ۿ�û ���� ���ѽð�(1800��[30��])�̻� �����.���� ���� ó��
	BOOL							_close_idleclient;

	// �̼��� ���� ó�� �ð�
	int								_expire_time;

	// ��� ���� 
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

		// KT�� ���� ��� ����
		BOOL doneflag;

		// KT�� ���� ��� ���� �ð�
		DWORD result_time;

		// Agent ��� �۽� �Ϸ�
		BOOL completeflag;

		// ���(�߼�)�ð�
		CTime reg_time;
	};

	HANDLE						_hExitEvent;

	BOOL							_bDoneThreadProc[2];

	// ������.
	vector<SEND_RESULT_INFO*>	_vtSendResult;

	// uidnumber�� SEND_RESULT_INFO ����
	map<CString, SEND_RESULT_INFO*>	_mapSendResult;

	// kshot_number�� uidnumber ����
	map<unsigned int, CString>	_mapKShotKey_UidSendNumber;

	// ���� ��� ����, ���� ����� ����, ���۽� �ε�
	CString						_SEND_RESULT_FILE;

	// ���� ���� ��� ����
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

	// ��� �˸� ��õ�
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


