#pragma once

// DBManager 명령 대상입니다.

#include "include/mysql/mysql.h"

// 발송큐 정보
#include "SendQueueInfo.h"

#include "ClientManager.h"

#include "WiseCanAgent.h"
#include "FaxAgent.h"
#include "HaomunAgent.h"
#include "LGUPlusAgent.h"

	/*
	1) my_ulonglong mysql_affected_rows(MYSQL* mysql)
		INSERT, UPDATE, DELETE 등의 query로 영향을 받은 ROW의 수를 리턴한다.
	14) unsigned int mysql_num_fields(MYSQL_RES*result) 혹은
		unsigned int mysql_num_fields(MYSQL* mysql)
		필드의 수를 리턴한다. 

	15) my_ulonglong mysql_num_rows(MYSQL_RES* result)
	result에 총 몇 개의 ROW가 있는지 리턴한다. query 수행 후 
	mysql_store_result()를 호출하였을 경우에만 사용할 수 있고,
	mysql_use_result()는 사용할 수 없다.
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

	// 디비 서버 정보
	CString			_server;
	CString			_id;
	CString			_pwd;
	CString			_dbname;
	CString			_dbport;

	vector<SEND_QUEUE_INFO*>	_vtDatas;

	volatile BOOL	_bExitFlag;
	volatile BOOL	_bDoneThreadFlag;
	volatile BOOL	_bDoneThreadFlag2;

	//응답 목록
	CString			_list_response;

	// 결과 목록
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

	// 초기화( mysql ) 
	bool			Init();
	bool			UnInit();

	bool			Connect();

	bool			Reconnect(MYSQL *mysql);

	// msgqueue id의 AUTO_INCREMENT 수정(중복키 에러 방지)
	bool			AlterAutoIncrement();

	// 잔액 삭감
	bool			AddEventLog_WithdrawBalance(CString uid, CString pid, int msgType, float price, int count, double balance, char* strRegDate);

	// 잔액 체크
	bool			CheckBalance(CString uid, int msgType, int count, float* price, double* balance, CString& pid, CString& line);

	// 사용자 로그인 처리
	bool			Login(CString id, CString pwd);

	// msgqueue에 저장
	
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

	// STATE 변경
	bool			ModifyState_REQUEST(CString idList);
	bool			ModifyState_REQUEST(CString id, CString send_number_list);
	bool			ModifyState_RESPONSE(CString idlist);

	int				LoadNonResultNotify(CString uid, vector<ClientManager::SEND_RESULT_INFO*>& vtList);
	bool			UpdateResultNotify(CString notifyList);

	// state 변경, Move
	bool			MoveToResultTable(CString list_kshot_number, int result, int telecom);

	// sendNo 얻기
	bool			GetSendNo(CString kshotNumber, CString& uid, CString& sendNo, CString& receiveNo, int& msgtype);

	// 실패에 대한 환불처리
	bool			refundBalance(CString uid, CString pid, int msgType, float price);

	// 일괄 환불처리
	BOOL			refundBatch();

	// 미수신결과 만료에 대한 일괄 실패 처리
	BOOL			FailForExpireBatch();

	BOOL			FailForExpire(int kshot_number, CString uid, CString pid, int msgtype, float price);

	// 미수신결과 만료에 대한 실패 처리
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


