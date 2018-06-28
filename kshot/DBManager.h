#pragma once

// DBManager ��� ����Դϴ�.

#include "include/mysql/mysql.h"

// �߼�ť ����
#include "SendQueueInfo.h"

#include "ClientManager.h"

#include "WiseCanAgent.h"
#include "FaxAgent.h"
#include "HaomunAgent.h"
#include "LGUPlusAgent.h"

	/*
	1) my_ulonglong mysql_affected_rows(MYSQL* mysql)
		INSERT, UPDATE, DELETE ���� query�� ������ ���� ROW�� ���� �����Ѵ�.
	14) unsigned int mysql_num_fields(MYSQL_RES*result) Ȥ��
		unsigned int mysql_num_fields(MYSQL* mysql)
		�ʵ��� ���� �����Ѵ�. 

	15) my_ulonglong mysql_num_rows(MYSQL_RES* result)
	result�� �� �� ���� ROW�� �ִ��� �����Ѵ�. query ���� �� 
	mysql_store_result()�� ȣ���Ͽ��� ��쿡�� ����� �� �ְ�,
	mysql_use_result()�� ����� �� ����.
	*/


class DBManager : public CObject
{
public:
	DBManager();
	virtual ~DBManager();

	////////////////////////////////////////////////////////////////////// DATA
	
	MYSQL			_mysql; 
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	// ��� ���� ����
	CString			_server;
	CString			_id;
	CString			_pwd;
	CString			_dbname;
	CString			_dbport;

	vector<SEND_QUEUE_INFO*>	_vtDatas;

	volatile BOOL	_bExitFlag;
	volatile BOOL	_bDoneThreadFlag;
	volatile BOOL	_bDoneThreadFlag2;

	//���� ���
	CString			_list_response;

	// ��� ���
	class LIST_RESULT
	{
	public:
		int			_result;
		int			_telecom;
		CString	_list_khost_number;
	};
	vector<LIST_RESULT*>	_vt_result_list;

	CCriticalSection	_cs;
	CCriticalSection	_cs2;

	map<CString, int>		_mapRequestCount;

	WiseCanAgent		_agentDelegator;

	FaxAgent			_faxDelegator;

	HaomunAgent		_haomunDelegator;

	LGUPlusAgent		_lguplusDelegator;


	////////////////////////////////////////////////////////////////////// METHOD

	// �ʱ�ȭ( mysql ) 
	bool			Init();
	bool			UnInit();

	bool			Connect();

	bool			Reconnect(MYSQL *mysql);

	// msgqueue id�� AUTO_INCREMENT ����(�ߺ�Ű ���� ����)
	bool			AlterAutoIncrement();

	// �ܾ� �谨
	bool			AddEventLog_WithdrawBalance(CString uid, CString pid, int msgType, float price, int count, double balance, char* strRegDate);

	// �ܾ� üũ
	bool			CheckBalance(CString uid, int msgType, int count, float* price, double* balance, CString& pid, CString& line);

	// ����� �α��� ó��
	bool			Login(CString id, CString pwd);

	// msgqueue�� ����
	
	int				InsertBatch(CString query);

	void			DeleteSendDataByIterator(vector<SEND_QUEUE_INFO*>& sentDatas);
	void			DeleteSendDataByArray(vector<SEND_QUEUE_INFO*>& sentDatas);
	void			DeleteSendDataByArrayV2(vector<SEND_QUEUE_INFO*>& sentDatas);
	void			DeleteSendDataByAdvance(vector<SEND_QUEUE_INFO*>& sentDatas);

	void			FlushSendData(vector<SEND_QUEUE_INFO*>& sentDatas);


	int				GetSendData(vector<SEND_QUEUE_INFO*>& sendData);
	
	int				AddSendData(CString id, unsigned int kshot_number, CString send_number, int msgtype, CString callbackNo, CString receiveNo, CString subject,
								CString msg, CString filename, CString filepath, int filesize, char isReserved, char* reservedTime, char* szRegTime);

	int				AddSendData(CString uid, vector<SEND_QUEUE_INFO*> vtRequest);

	int				InsertDirectData(CString id, CString send_number, int msgtype, CString callbackNo, CString receiveNo, CString subject, 
								CString msg, CString filename, CString filepath, int filesize, char isReserved, char* reservedTime);
	
	int				FindFileId(CString id, CString filepath);
	int				FindFileIdSetValue(SEND_QUEUE_INFO* info);

	bool			GetQueueData(vector<SEND_QUEUE_INFO*>& sendQueue);

	bool			DeleteReservedMsgQueue(CString idList);

	// STATE ����
	bool			ModifyState_REQUEST(CString idList);
	bool			ModifyState_REQUEST(CString id, CString send_number_list);
	bool			ModifyState_RESPONSE(CString idlist);

	int				LoadNonResultNotify(CString uid, vector<ClientManager::SEND_RESULT_INFO*>& vtList);
	bool			UpdateResultNotify(CString notifyList);

	// state ����, Move
	bool			MoveToResultTable(CString list_kshot_number, int result, int telecom);

	// sendNo ���
	bool			GetSendNo(CString kshotNumber, CString& uid, CString& sendNo, CString& receiveNo, int& msgtype);

	// ���п� ���� ȯ��ó��
	bool			refundBalance(CString uid, CString pid, int msgType, float price);

	// �ϰ� ȯ��ó��
	BOOL			refundBatch();

	// �̼��Ű�� ���ῡ ���� �ϰ� ���� ó��
	BOOL			FailForExpireBatch();

	BOOL			FailForExpire(int kshot_number, CString uid, CString pid, int msgtype, float price);

	// �̼��Ű�� ���ῡ ���� ���� ó��
	bool			FailForExpireSMS(CString kshot_number);

	bool			checkNReconnect(MYSQL*mysql = NULL);

	BOOL			IsExistResponse(CString& responseList);
	void			AddResponse(CString id);
	void			ClearResponse();

	BOOL			IsExistResult();
	void			AddResult(CString kshot_number, int result, int telecom);
	void			ClearResult();

	void			InsertMessageTable(vector<SEND_QUEUE_INFO*>& sendData);
};


