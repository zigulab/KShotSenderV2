// DBManager.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "DBManager.h"

#include "./include/mysql/errmsg.h"

#pragma comment(lib,"./lib/mysql/libmysql.lib")



// DBManager

CCriticalSection	g_csSendData;


UINT SaveResultProc(LPVOID lpParameter)
{
	DBManager*	_dbManager = (DBManager*)lpParameter;

	while (1)
	{
		if (_dbManager->_bExitFlag)
			break;

		BOOL isChanged = FALSE;

		CString responseList;
		// 응답리스트에 값이 있으면
		if (_dbManager->IsExistResponse(responseList))
		{
			isChanged = TRUE;

			// 응답 리스트를 넘겨서 업데이트 처리
			_dbManager->ModifyState_RESPONSE(responseList);

			// 응답리스트 제거
			_dbManager->ClearResponse();
		}

		vector<DBManager::LIST_RESULT> resultList;
		// 결과리스트에 값이 있으면

		_dbManager->_cs2.Lock();

		if (_dbManager->IsExistResult())
		{
			isChanged = TRUE;

			// 결과 리스트를 넘겨서 업데이트 처리
			//vector<DBManager::LIST_RESULT>::iterator it = resultList.begin();
			//for (int i = 0; it != resultList.end(); it++)
			//{
			//	DBManager::LIST_RESULT result_list = *it;
			//	_dbManager->MoveToResultTable(result_list._list_khost_number, result_list._result, result_list._telecom);
			//}

			vector<DBManager::LIST_RESULT*>::iterator pos = _dbManager->_vt_result_list.begin();
			for (; pos  != _dbManager->_vt_result_list.end(); pos++)
			{
				DBManager::LIST_RESULT* result_list = *pos;
				if( result_list->_list_khost_number.IsEmpty() == FALSE )
					_dbManager->MoveToResultTable(result_list->_list_khost_number, result_list->_result, result_list->_telecom);
			}

			// 응답리스트 제거
			_dbManager->ClearResult();
		}

		_dbManager->_cs2.Unlock();

		if (isChanged) {
			PostThreadMessage(g_mainTid, WM_STATISTIC_RECAL, 0, 0);
		}


		DWORD ret = WaitForSingleObject(g_hDBEvent2, 1500);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("SaveDataProc(), WaitForSingleObject Fail"));
			break;
		}
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("SaveResultProc(), 종료됨."));

	_dbManager->_bDoneThreadFlag2 = TRUE;

	return 0;
}

UINT SaveDataProc(LPVOID lpParameter)
{
	DBManager*	_dbManager = (DBManager*)lpParameter;

	while(1)
	{
		if( _dbManager->_bExitFlag )
			break;

		// 10분(600초) 간격으로 DB 연결유지 확인
		int timeout_inerval = 600 * 1000;  // millisec

		DWORD ret = WaitForSingleObject(g_hDBEvent, timeout_inerval);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("SaveDataProc(), WaitForSingleObject Fail"));
			break;
		}

		if (ret == WAIT_TIMEOUT) {
			if (_dbManager->checkNReconnect() == false) {
				Log(CMdlLog::LEVEL_ERROR, _T("SaveDataProc(), DB연결에 문제가 있습니다."));
			}
			continue;
		}


		vector<SEND_QUEUE_INFO*> vtSendData;

		int prevCount = 0, curCount = 0;

		// 저장된 버퍼을 읽어 들임
		curCount = _dbManager->GetSendData(vtSendData);

		if (curCount <= 0) continue;
	
		/*
		// 한번에 최대한 많이 insert하기 위해 한번 더 읽어들임.
		do
		{
			prevCount = curCount;

			Sleep(10);

			curCount = _dbManager->GetSendData(vtSendData);

			// 변화가 없으면 탈출
		} while (curCount != prevCount );
		*/

		//TRACE(_T("SaveDataProc에서 Insert전에 loop count:%d\n"), loopcnt);

		CString query;
		query.Format(_T("insert into msgqueue(uid, sendNo, kind, callbackNo, receiveNo, subject, msg, \
									mmsfile_id, registTime, isReserved, reservedTime) values"));
			
		int loopcnt = 0;

		vector<SEND_QUEUE_INFO*>::iterator it = vtSendData.begin();
		for( ; it != vtSendData.end(); it++)
		{
			SEND_QUEUE_INFO* info = *it;

			CString OneRowValue;
			CString reserveTimeValue;

			if( toupper(info->isReserved) == 'Y' )
			{
				reserveTimeValue.Format(_T("\'%s\'"), info->reservedTime);
			}
			else
				reserveTimeValue = "NULL";

			CString szFileId = "NULL";
			if( info->msgtype == 2 )
			{
				int file_id = -1;
				if( (file_id = _dbManager->FindFileId(info->uid, info->filepath)) != -1 )
					szFileId.Format(_T("%d"), file_id);
			}

			// 문장내 ' =>\' 변경
			CString smsMessage = info->msg;
			smsMessage.Replace("'", "\\'");

			CString smsSubject = info->subject;
			if( info->msgtype > 0 )
				smsSubject.Replace("'", "\\'");

			OneRowValue.Format(_T(" ('%s','%s',%d,'%s','%s','%s','%s',%s,%s,'%c',%s)"),
				info->uid,
				info->send_number,
				info->msgtype,
				info->callback,
				info->receive,
				smsSubject.GetBuffer(),
				smsMessage.GetBuffer(),
				szFileId,
				info->registTime,
				info->isReserved,
				reserveTimeValue);

			query += OneRowValue;
			query += ",";

			loopcnt++;
		}
		query.Delete(query.GetLength()-1);

		_dbManager->InsertBatch(query);

		CString logmsg;
		logmsg.Format(_T("SaveDataProc(), [msg] 요청버퍼 => DB에 저장(count:%d)"), loopcnt);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		// 발송시작을 알림
		SetEvent(g_hSendEvent);

		_dbManager->FlushSendData(vtSendData);
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("SaveDataProc(), 종료됨."));

	_dbManager->_bDoneThreadFlag = TRUE;

	return 0;
}

void DBManager::InsertMessageTable(vector<SEND_QUEUE_INFO*>& sendData)
{
	CString queue_query, result_query;

	CString queue_query_fmt, result_query_fmt;
	queue_query_fmt.Format(_T("insert into msgQueue(send_queue_id, uid, sendNo, kind, callbackNo, receiveNo, subject, msg, \
									mmsfile_id, registTime, isReserved, reservedTime, state, result) values"));

	result_query_fmt.Format(_T("insert into msgResult(send_queue_id, uid, sendNo, kind, callbackNo, receiveNo, subject, msg, \
									mmsfile_id, registTime, sendTime, isReserved, reservedTime, state, result) values"));

	queue_query = queue_query_fmt;
	result_query = result_query_fmt;

	int instant_cnt = 0, reserved_cnt = 0;

	int size = sendData.size();
	int n = 0;
	for (; n < size; )
	{
		SEND_QUEUE_INFO* info = sendData[n];

		CString OneRowValue;
		CString reserveTimeValue;
		int state_val = 1;

		BOOL reserved = FALSE;
		if (toupper(info->isReserved) == 'Y')
			reserved = TRUE;

		if (reserved)
		{
			reserveTimeValue.Format(_T("\'%s\'"), info->reservedTime);
			state_val = 0;
		}
		else
			reserveTimeValue = "NULL";

		CString szFileId = "NULL";
		if (info->msgtype == 2)
		{
			int file_id = -1;
			if ((file_id = FindFileIdSetValue(info)) != -1)
				szFileId.Format(_T("%d"), file_id);
		}

		// 문장내 ' =>\' 변경
		CString smsMessage = info->msg;
		smsMessage.Replace("'", "\\'");

		CString smsSubject = info->subject;
		if (info->msgtype > 0)
			smsSubject.Replace("'", "\\'");

		if (reserved) 
		{
			OneRowValue.Format(_T(" (%d,'%s','%s',%d,'%s','%s','%s','%s',%s,%s,'%c',%s,%d,%d)"),
				info->kshot_number,
				info->uid,
				info->send_number,
				info->msgtype,
				info->callback,
				info->receive,
				smsSubject.GetBuffer(),
				smsMessage.GetBuffer(),
				szFileId,
				info->registTime,
				info->isReserved,
				reserveTimeValue,
				state_val,				// 미발송, 발송
				-1							// 미수신
			);

			queue_query += OneRowValue;
			queue_query += ",";
			reserved_cnt++;
		}
		else {
			OneRowValue.Format(_T(" (%d,'%s','%s',%d,'%s','%s','%s','%s',%s,%s,NOW(),'%c',%s,%d,%d)"),
				info->kshot_number,
				info->uid,
				info->send_number,
				info->msgtype,
				info->callback,
				info->receive,
				smsSubject.GetBuffer(),
				smsMessage.GetBuffer(),
				szFileId,
				info->registTime,
				info->isReserved,
				reserveTimeValue,
				state_val,				// 미발송, 발송
				-1							// 미수신
			);

			result_query += OneRowValue;
			result_query += ",";
			instant_cnt++;
		}
		
		n++;

		if (n % 1000 == 0)
		{
			if (n % 50000 == 0)
			{
				DWORD begin = GetTickCount();

				if (reserved_cnt > 0) {
					queue_query.Delete(queue_query.GetLength() - 1);
					InsertBatch(queue_query);

					queue_query = queue_query_fmt;
					reserved_cnt = 0;
				}

				if (instant_cnt > 0) {
					result_query.Delete(result_query.GetLength() - 1);
					InsertBatch(result_query);

					result_query = result_query_fmt;

					instant_cnt = 0;
				}
				DWORD end = GetTickCount();

				CString logmsg;
				logmsg.Format(_T("[msg] DB Insert(count:%d, elapseTime:%d)"), n, end-begin);
				Log(CMdlLog::LEVEL_EVENT, logmsg);

				Sleep(0);
			}
			else
			{
				CString logmsg;
				logmsg.Format(_T("[msg] DB Insert(count:%d)"), n);
				Log(CMdlLog::LEVEL_EVENT, logmsg);
			}
		}
	}

	CString logmsg;
	logmsg.Format(_T("[msg] DB Insert(count:%d)"), n);
	Log(CMdlLog::LEVEL_EVENT, logmsg);

	if (reserved_cnt > 0) {
		queue_query.Delete(queue_query.GetLength() - 1);
		InsertBatch(queue_query);
	}

	if (instant_cnt > 0) {
		result_query.Delete(result_query.GetLength() - 1);
		InsertBatch(result_query);
	}

}

DBManager::DBManager()
	:_agentDelegator(this),
	_faxDelegator(this),
	_haomunDelegator(this),
	_lguplusDelegator(this)
{
	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;
}

DBManager::~DBManager()
{
}


// DBManager 멤버 함수

bool DBManager::Init()
{
	Log(CMdlLog::LEVEL_EVENT, _T("DBManager 초기화"));

	_bExitFlag		 = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;

	_server		= g_config.db.server;
	_id			= g_config.db.id;
	_pwd		= g_config.db.password;
	_dbname		= g_config.db.dbname;
	_dbport		= g_config.db.dbport;

	Log(CMdlLog::LEVEL_EVENT, _T("Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);
	
	if (!Connect()) return false;

	AlterAutoIncrement();

	if (!_agentDelegator.Init()) return false;
	
	if (!_faxDelegator.Init()) return false;

	if (!_haomunDelegator.Init()) return false;

	if (!_lguplusDelegator.Init()) return false;

	return true;
}

bool DBManager::UnInit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("DBManager, 종료 시작"));

	Log(CMdlLog::LEVEL_EVENT, _T("mysql_close()"));

	mysql_close(&_mysql);

	_agentDelegator.UnInit();

	_faxDelegator.UnInit();

	_haomunDelegator.UnInit();

	_lguplusDelegator.UnInit();

	_bExitFlag = TRUE;

	SetEvent(g_hDBEvent);
	while( !_bDoneThreadFlag )
		::Sleep(10);


	while (!_bDoneThreadFlag2)
		::Sleep(10);

	vector<SEND_QUEUE_INFO*>::iterator pos = _vtDatas.begin();
	for(; pos != _vtDatas.end(); pos++ )
	{
		SEND_QUEUE_INFO *orginfo = *pos;
		delete orginfo;
	}
	_vtDatas.clear();

	vector<LIST_RESULT*>::iterator it = _vt_result_list.begin();
	for (int i = 0; it != _vt_result_list.end(); it++)
	{
		LIST_RESULT* result_list = *it;
		delete result_list;
	}
	_vt_result_list.clear();

	Log(CMdlLog::LEVEL_EVENT, _T("DBManager, 내부 데이타 삭제"));

	return true;
}

bool DBManager::Connect()
{
	//if(!mysql_real_connect(&_mysql, DB_HOST, DB_USER, DB_PASS, DB_NAME ,3306,0,0))

	CString msg;

	int port = _ttoi(_dbport);

	if(!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		
		msg.Format(_T("Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"),_server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("Mysql 연결(%s,%s//%s,%s,%s) 성공"),_server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}


	g_hDBEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_hDBEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);

	Log(CMdlLog::LEVEL_EVENT, _T("DBManager::SaveDataProc() Running..."));

	CWinThread* pThread = AfxBeginThread(SaveDataProc, this);

	CWinThread* pThread2 = AfxBeginThread(SaveResultProc, this);

	return true;
}

bool DBManager::Reconnect(MYSQL *mysql)
{
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		CString msg;
		msg.Format(_T("Mysql 재연결 실패(%s,%s//%s,%s,%s) error(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}

	Log(CMdlLog::LEVEL_EVENT, _T("Mysql 재연결 성공!"));

	return true;
}

bool DBManager::AlterAutoIncrement()
{
	g_cs.Lock();

	long queue_max_id = -1, result_max_id = -1;
			
	// if ( isnull(birthday), '-', birthday )
	CString query;
	query.Format("select max(send_queue_id) maxid from msgqueue");
	if( mysql_query(&_mysql, query) == 0 )
	{ 
		if((_res = mysql_store_result(&_mysql)) != NULL)
		{ 
			if((_row = mysql_fetch_row(_res)) != NULL) {
				if( _row[0] == NULL )
					queue_max_id = 0;
				else
					queue_max_id = _ttoi(_row[0]);
			}

			mysql_free_result(_res);
		}	

		query.Format("select max(send_queue_id) maxid from msgresult");
		if( mysql_query(&_mysql, query) == 0 )
		{ 
			if((_res = mysql_store_result(&_mysql)) != NULL)
			{ 
				if((_row = mysql_fetch_row(_res)) != NULL) {
					if( _row[0] == NULL )
						result_max_id = 0;
					else
						result_max_id = _ttoi(_row[0]);
				}

				mysql_free_result(_res);
			}	
		}
	}
	else 
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::AlterAutoIncrement(), mysql_query(%s) error(%s)"), query, mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	if (queue_max_id == -1 || result_max_id == -1) {
		g_cs.Unlock();
		return false;
	}

	long max_id = max(queue_max_id, result_max_id);

	g_kshot_send_number = max_id;

	/*
	query.Format(_T("alter table msgqueue auto_increment=%d"), max_id);
	if(mysql_query(&_mysql, query))
	{ 
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::AlterAutoIncrement() mysql_query2 error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}
	*/

	g_cs.Unlock();

	return true;
}

// 전송 기록, 잔액 삭감(인출)
bool DBManager::AddEventLog_WithdrawBalance(CString uid, CString pid, int msgType, float price, int count, double balance, char* strRegDate)
{
	g_cs.Lock();

	mysql_autocommit(&_mysql, false); // start transaction 

	float cost = price * count;

	double chargedBalance = balance - cost;

	CString query;
	//query.Format(_T("insert into sendLog values(NULL, '%s', %d, %f, %d, NOW(), NULL, NULL)"), uid, msgType, price, count);
	query.Format(_T("insert into sendLog(seq, uid, msgType, price, sendCount, sendDate, beforeBalance, chargedBalance) values(NULL, '%s', %d, %f, %d, %s, %f, %f)"), uid, msgType, price, count, strRegDate, balance, chargedBalance);

	if (mysql_query(&_mysql, query))
	{
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::AddEventLog_WithdrawBalance(), mysql_query error(%s)"), mysql_error(&_mysql));
		
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	if( pid.IsEmpty() )
		query.Format(_T("update member set balance = balance-%f where uid = '%s'"), cost, uid);
	else {
		// pid: 메인id에서 삭감한다.
		query.Format(_T("update member set balance = balance-%f where uid = '%s'"), cost, pid);
	}

	if (mysql_query(&_mysql, query))
	{
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::AddEventLog_WithdrawBalance(), mysql_query2 error(%s)"), mysql_error(&_mysql));
		
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 

	g_cs.Unlock();

	return true;
}

// 잔액 체크
bool DBManager::CheckBalance(CString uid, int msgType, int count, float* price, double* balance, CString& pid, CString& line)
{
	g_cs.Lock();

	bool retval = false;

	CString query;
	query.Format(_T("select sms_price, lms_price, mms_price, fax_price, balance, pid, smsline, lmsline, mmsline, faxline from member where uid='%s' and use_state=0"), uid);

	// mysql_query == 0 이면 성공
	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::CheckBalance(), mysql_query(%s) error(%s)"), query, mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	if ((_res = mysql_store_result(&_mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::CheckBalance(), mysql_store_result error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	if ((_row = mysql_fetch_row(_res)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::CheckBalance(), mysql_fetch_row empty(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}


	// 비용계산
	float cost = -1;

	if (msgType == 0)
	{
		*price = _ttof(_row[0]);
		line = _row[6];
	}
	else if (msgType == 1)
	{
		*price = _ttof(_row[1]);
		line = _row[7];
	}
	else if (msgType == 2)
	{
		*price = _ttof(_row[2]);
		line = _row[8];
	}
	else if (msgType == 3)
	{
		*price = _ttof(_row[3]);
		line = _row[9];
	}

	cost = *price * count;

	*balance = _ttof(_row[4]);

	CString parentId = _row[5];

	if (parentId.IsEmpty()) 
	{
		
		if (*price != -1 && cost <= *balance)
			retval = true;

		mysql_free_result(_res);
	}
	else 
	{
		mysql_free_result(_res);

		pid = parentId;

		query.Format(_T("select balance from member where uid='%s'"), pid);

		if (mysql_query(&_mysql, query) == 0)
		{
			if ((_res = mysql_store_result(&_mysql)) != NULL)
			{
				if ((_row = mysql_fetch_row(_res)) != NULL)
				{
					*balance = _ttof(_row[0]);

					if (*price != -1 && cost <= *balance)
						retval = true;
				}

				mysql_free_result(_res);
			}
		}
	}
			
	g_cs.Unlock();

	return retval;
}

// 사용자 로그인 처리
bool DBManager::Login(CString id, CString pwd)
{
	g_cs.Lock();

	//if( mysql_query(&_mysql,"set names euckr") ) //한글 인식
	//{
	//	TRACE(mysql_error(&_mysql));
	//	TRACE(_T("한글 설정 실패\n"));
	//	g_cs.Unlock();
	//	return false;
	//}

	bool retval = false;

	CString query;
	query.Format(_T("select * from member where uid='%s' and password='%s'"), id, pwd);
	
	// mysql_query == 0 이면 성공
	if(mysql_query(&_mysql, query) == 0 )
	{ 
		if( (_res = mysql_store_result(&_mysql)) != NULL )
		{ 
			if( (_row = mysql_fetch_row(_res)) != NULL )
			{
				// 출력창에 디버깅 용으로 
				//TRACE("mysql에 받은 값입니다 : no:%s uid:%s name:%s\n", _row[0], _row[2], _row[4]);
				retval = true;
			}
			else
				Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::Login(), mysql_fetch_row empty"));

			mysql_free_result(_res);
		}
		else
			Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::Login(), mysql_store_result null"));
	}
	else
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::Login(), mysql_query(%s) error(%s)"), query, mysql_error(&_mysql));

	g_cs.Unlock();

	return retval;
}

int	DBManager::InsertBatch(CString query)
{
	g_cs.Lock();

	if( mysql_query(&_mysql,"set names euckr") )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertBatch(), set names euckr error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	//if( mysql_send_query(&_mysql, query, strlen(query)) )
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertBatch(), mysql_error(%s)"),mysql_error(&_mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertBatch(), query(%s)"), query);
		g_cs.Unlock();
		return -1;
	}
	
	g_cs.Unlock();

	//query.Format(_T("select  LAST_INSERT_ID() as lastnum"));
	//int retval = _ttoi(_row[0]);

	return 1;
}

int	DBManager::GetSendData(vector<SEND_QUEUE_INFO*>& sendData)
{
	g_csSendData.Lock();

	// 최대 5만건 단위로 처리한다.
	//int data_size = _vtDatas.size();
	//if (data_size > 50000) data_size = 50000;
	//sendData.assign(_vtDatas.begin(), _vtDatas.begin()+ data_size);

	// 복사한다.
	sendData = _vtDatas;

	// 기존 큐는 삭제한다.
	_vtDatas.clear();
	_mapRequestCount.clear();

	// Shuffle을 판단한다.
	// 복사된 버퍼에서 	1만건 이상이 있고,	뒤에 발송 건수가 있을 때

	BOOL doShuffle = FALSE;

	CString uid, prev_uid;

	BOOL existMsss = FALSE;
	int  count = 0, prev_count = 0;

	int size = sendData.size();

	TRACE(_T("GetSendData(%d)\n"), size);

	for (int i = 0; i < size; i++)
	{
		uid = sendData[i]->uid;

		if (i > 0 && uid != prev_uid) 
		{
			TRACE(_T("uid count(%d)\n"), count);
			if (count > 10000) {
				if (existMsss) {
					doShuffle = TRUE;
					break;
				}
				else
					existMsss = TRUE;
			}
			else if (count >= 1000) {
				if (existMsss) {
					doShuffle = TRUE;
					break;
				}
			}
			count = 0;
		}

		count++;

		prev_uid = uid;
	}

	if (!doShuffle && count > 0)
	{
		TRACE(_T("uid count(%d)\n"), count);
		if (count >= 1000) {
			if (existMsss) 
				doShuffle = TRUE;
		}
	}

	if (doShuffle) 
	{
		//DWORD b = GetTickCount();
		
		srand(time(NULL));
		random_shuffle(sendData.begin(), sendData.end());

		//DWORD e = GetTickCount();
		//TRACE(_T("Shuffle처리 완료(%d), time:%d\n"), size, e-b);
	}

	g_csSendData.Unlock();

	return size;
}

void DBManager::DeleteSendDataByIterator(vector<SEND_QUEUE_INFO*>& sentDatas)
{
	// 속도가 상당히 느림. 사용하지 않아야 함.(참고만)

	g_csSendData.Lock();

	ULONGLONG b = GetTickCount64();

	///*
	int n = 0;
	int count = 0;
	CString uid, prev_uid;

	vector<SEND_QUEUE_INFO*>::iterator it = sentDatas.begin();
	for (; it != sentDatas.end(); it++)
	{
		SEND_QUEUE_INFO *qinfo = *it;
		uid = qinfo->uid;

		vector<SEND_QUEUE_INFO*>::iterator pos = _vtDatas.begin();
		for (; pos != _vtDatas.end(); pos++)
		{
			SEND_QUEUE_INFO *orginfo = *pos;
			if (orginfo == qinfo)
			{
				delete orginfo;
				_vtDatas.erase(pos);
				break;
			}
		}

		if (n > 0 && uid != prev_uid) {
			map<CString, int>::iterator pos2 = _mapRequestCount.find(prev_uid);
			if (pos2 != _mapRequestCount.end()) {
				int cnt = pos2->second;
				_mapRequestCount[uid] = (cnt - count);
			}
			else
				ASSERT(0);

			count = 0;
		}
		else
			count++;

		n++;
		prev_uid = uid;

		if (n % 1000 == 0) Sleep(0);
	}

	if (count > 0) {
		map<CString, int>::iterator pos2 = _mapRequestCount.find(prev_uid);
		if (pos2 != _mapRequestCount.end()) {
			int cnt = pos2->second;
			_mapRequestCount[uid] = (cnt - count);
		}
		else
			ASSERT(0);
	}

	g_csSendData.Unlock();
}

void DBManager::DeleteSendDataByArray(vector<SEND_QUEUE_INFO*>& sentDatas)
{
	// 속도는 괜찮고.. 안정적인 처리.

	g_csSendData.Lock();

	int count = 0;
	CString uid, prev_uid;

	int size = sentDatas.size();
	for (int i = 0; i < size;)
	{
		uid = sentDatas[i]->uid;

		vector<SEND_QUEUE_INFO*>::iterator it = find(_vtDatas.begin(), _vtDatas.end(), sentDatas[i]);
		if (it != _vtDatas.end())
		{
			delete *it;
			_vtDatas.erase(it);
		}
		else
			ASSERT(0);

		if (i > 0 && uid != prev_uid) {
			map<CString, int>::iterator pos = _mapRequestCount.find(prev_uid);
			if (pos != _mapRequestCount.end()) {
				int cnt = pos->second;
				_mapRequestCount[uid] = (cnt - count);
			}
			else
				ASSERT(0);

			count = 0;
		}
		else
			count++;

		i++;
		prev_uid = uid;

		if (i % 1000 == 0) Sleep(0);
	}

	if (count > 0) {
		map<CString, int>::iterator pos2 = _mapRequestCount.find(prev_uid);
		if (pos2 != _mapRequestCount.end()) {
			int cnt = pos2->second;
			_mapRequestCount[uid] = (cnt - count);
		}
		else
			ASSERT(0);
	}

	g_csSendData.Unlock();
}

void DBManager::DeleteSendDataByArrayV2(vector<SEND_QUEUE_INFO*>& sentDatas)
{
	// 속도는 괜찮고.. 안정적인 처리.

	g_csSendData.Lock();

	int n = 0;
	int count = 0;
	CString uid, prev_uid;

	int size = sentDatas.size();
	for (int i = 0; i < size; )
	{
		SEND_QUEUE_INFO* info = sentDatas[i];
		delete info;

		i++;
		if (i % 1000 == 0) Sleep(0);
	}

	g_csSendData.Unlock();
}

void DBManager::DeleteSendDataByAdvance(vector<SEND_QUEUE_INFO*>& sentDatas)
{
	// 속도는 빠르나. 대용량 병렬처리에서 Data가 Suffle되기 때문에 Exception 발생, 그 외에 호출되어야 함.

	g_csSendData.Lock();

	int size = sentDatas.size();

	vector<SEND_QUEUE_INFO*>::iterator it = _vtDatas.begin();

	vector<SEND_QUEUE_INFO*>::iterator it2 = it;
	advance(it2, size);

	_vtDatas.erase(it, it2);

	int count = 0;
	CString uid, prev_uid;

	for (int i = 0; i < size;) 
	{
		SEND_QUEUE_INFO* info = sentDatas[i];

		uid = info->uid;

		delete info;

		if (i > 0 && uid != prev_uid) {
			map<CString, int>::iterator pos = _mapRequestCount.find(prev_uid);
			if (pos != _mapRequestCount.end()) {
				int cnt = pos->second;
				_mapRequestCount[uid] = (cnt - count);
			}
			else
				ASSERT(0);

			count = 0;
		}
		else
			count++;

		i++;
		prev_uid = uid;

		if (i % 1000 == 0) Sleep(0);
	}

	if (count > 0) {
		map<CString, int>::iterator pos2 = _mapRequestCount.find(prev_uid);
		if (pos2 != _mapRequestCount.end()) {
			int cnt = pos2->second;
			_mapRequestCount[uid] = (cnt - count);
		}
		else
			ASSERT(0);
	}

	g_csSendData.Unlock();
}

void DBManager::FlushSendData(vector<SEND_QUEUE_INFO*>& sentDatas)
{
	//ULONGLONG b = GetTickCount64();

	/*
	int mass_savedbase = 10000;

	int isValidCount = 0;
	BOOL isMassOneMore = FALSE;

	map<CString, int>::iterator it = _mapRequestCount.begin();
	for (; it != _mapRequestCount.end(); it++) 
	{
		if (it->second > 0) 
		{
			isValidCount++;
			if (it->second > mass_savedbase) {
				isMassOneMore = TRUE;
				break;
			}
		}
	}

	if(isValidCount > 1 && isMassOneMore)
		DeleteSendDataByArray(sentDatas); 
	else
		DeleteSendDataByAdvance(sentDatas);
	*/

	DeleteSendDataByArrayV2(sentDatas);

	//ULONGLONG e = GetTickCount64();
	//TRACE(_T("GetResult() Elapse Time:%d\n"), e - b);
}

int DBManager::FindFileId(CString id, CString filepath)
{
	g_cs.Lock();

	int retval = -1;

	if( mysql_query(&_mysql,"set names euckr") )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FindFileId(), set names euckr error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return retval;
	}

	// 첨부파일 오류
	if( filepath.IsEmpty() ) {
		g_cs.Unlock();
		return retval;
	}
	
	CString query;
	query.Format(_T("select * from mmsinfo where uid='%s' and fileFullPath='%s'"), id, filepath);

	if( mysql_query(&_mysql, query) == 0 )
	{ 
		if((_res = mysql_store_result(&_mysql)) != NULL)
		{ 
			if((_row = mysql_fetch_row(_res)) != NULL)
			{
				//TRACE("mysql에 받은 값입니다 : no:%s uid:%s name:%s\n", _row[0], _row[2], _row[4]);
				retval = _ttoi(_row[0]);
			}				

			mysql_free_result(_res);
		}	
	}

	g_cs.Unlock();

	return retval;
}

int DBManager::FindFileIdSetValue(SEND_QUEUE_INFO* info)
{
	g_cs.Lock();

	CString id = info->uid;
	CString filepath = info->filepath;

	int retval = -1;

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FindFileIdSetValue(), set names euckr error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return retval;
	}

	// 첨부파일 오류
	if (filepath.IsEmpty()) {
		g_cs.Unlock();
		return retval;
	}

	CString query;
	query.Format(_T("select * from mmsinfo where uid='%s' and fileFullPath='%s'"), id, filepath);

	if (mysql_query(&_mysql, query) == 0)
	{
		if ((_res = mysql_store_result(&_mysql)) != NULL)
		{
			if ((_row = mysql_fetch_row(_res)) != NULL)
			{
				//TRACE("mysql에 받은 값입니다 : no:%s uid:%s name:%s\n", _row[0], _row[2], _row[4]);
				if (_row[0] != NULL)
					retval = _ttoi(_row[0]);

				if (_row[1] != NULL)
					//strcpy(info->filename, _row[1]);
					info->filename = _row[1];

				if (_row[2] != NULL)
					//strcpy(info->filepath, _row[2]);
					info->filepath = _row[2];

				if (_row[3] != NULL)
					info->filesize = _ttoi(_row[3]);
			}

			mysql_free_result(_res);
		}
	}

	g_cs.Unlock();

	return retval;
}

int	DBManager::AddSendData(CString id, unsigned int kshot_number, CString send_number, int msgtype, CString callbackNo, CString receiveNo, CString subject,
								CString msg, CString filename, CString filepath, int filesize, char isReserved, char* reservedTime, char* szRegTime)
{
	SEND_QUEUE_INFO* info = new SEND_QUEUE_INFO();
	
	/*
	memset(info, 0x00, sizeof(info));
	strcpy(info->uid, id.GetBuffer());
	info->msgtype = msgtype;
	strcpy(info->send_number,send_number.GetBuffer());
	strcpy(info->callback,callbackNo.GetBuffer());
	strcpy(info->receive,receiveNo.GetBuffer());
	//strcpy(info->subject,subject.GetBuffer());
	info->subject = subject;
	//strcpy(info->msg,msg.GetBuffer());
	info->msg = msg;
	//strcpy(info->filename,filename.GetBuffer());
	info->filename = filename;
	//strcpy(info->filepath,filepath.GetBuffer());
	info->filepath = filepath;
	info->filesize = filesize;
	info->kshot_number = kshot_number;
	info->isReserved = isReserved;
	strcpy(info->reservedTime,reservedTime);
	strcpy(info->registTime, szRegTime);
	*/

	info->uid = id;
	info->msgtype = msgtype;
	info->send_number = send_number;
	info->callback = callbackNo;
	info->receive = receiveNo;
	info->subject = subject;
	info->msg = msg;
	info->filename = filename;
	info->filepath = filepath;
	info->filesize = filesize;
	info->kshot_number = kshot_number;
	info->isReserved = isReserved;
	info->reservedTime = reservedTime;
	info->registTime = szRegTime;


	g_csSendData.Lock();

	_vtDatas.push_back(info);
	
	g_csSendData.Unlock();

	return 1;
}

int	DBManager::AddSendData(CString uid, vector<SEND_QUEUE_INFO*> vtRequest)
{
	BOOL isExistMass = FALSE;
	BOOL isOtherMass = FALSE;

	// 기 저장된 대량 기준치 
	int mass_savedbase = 10000;
	int middle_requestbase = 1000;

	int request_count = vtRequest.size();

	g_csSendData.Lock();

	int count = _vtDatas.size();

	map<CString, int>::iterator it = _mapRequestCount.begin();
	for(; it != _mapRequestCount.end(); it++ ){
		if (it->second > mass_savedbase) {
			isExistMass = TRUE;
			if (it->first != uid) {
				isOtherMass = TRUE;
				break;
			}
		}
	}

	if (isExistMass && isOtherMass)
	{
		if (request_count >= middle_requestbase)
		{
			//DWORD b = GetTickCount();

			//int step_size = count / 10;
			//int step_size2 = request_count / 10;

			//vector<SEND_QUEUE_INFO*>::iterator it_begin = _vtDatas.begin();
			//for (int i = 0; i < 10; i++) {
			//	int insert_pos = (i + 1) * step_size;

			//	int src_begin_pos = i * step_size2;
			//	int src_end_pos = src_begin_pos + step_size2;
			//	_vtDatas.insert(it_begin + insert_pos, vtRequest.begin() + src_begin_pos, vtRequest.begin() + src_end_pos);
			//}

			//DWORD e = GetTickCount();
			//TRACE(_T("if (isExistMass && isOtherMass) Elapse Time: %d\n"), e - b);

			//_vtDatas.insert(_vtDatas.end(), vtRequest.begin(), vtRequest.end());
			//srand(time(NULL));
			//random_shuffle(_vtDatas.begin(), _vtDatas.end());		

			TRACE(_T("Shuffle처리 필요(%d, %d)\n"), count, request_count);

			// 일단 뒤에 붙이고 다음단계에서 Shuffle 처리한다.
			_vtDatas.insert(_vtDatas.end(), vtRequest.begin(), vtRequest.end());
		}
		else 
		{
			// 소량 추가는 앞에 붙인다.
			_vtDatas.insert(_vtDatas.begin(), vtRequest.begin(), vtRequest.end());
		}
	}
	else 
	{
		// 기 저장된 대량이 없으면 기본으로 뒤에 붙인다.
		_vtDatas.insert(_vtDatas.end(), vtRequest.begin(), vtRequest.end());
	}

	map<CString,int>::iterator it2 = _mapRequestCount.find(uid);
	if (it2 != _mapRequestCount.end()) {
		int n = it2->second;
		_mapRequestCount[uid] = n+ request_count;
	}
	else
		_mapRequestCount[uid] = request_count;

	g_csSendData.Unlock();

	return request_count;
}

int DBManager::InsertDirectData(CString id, CString send_number, int msgtype, CString callbackNo, CString receiveNo, CString subject, 
								CString msg, CString filename, CString filepath, int filesize, char isReserved, char* reservedTime)
{
	g_cs.Lock();

	if( mysql_query(&_mysql,"set names euckr") )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertDirectData(), set names euckr error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	CString mmsfile_id = "NULL";

	CString query;
	
	// MMS
	if( msgtype == 2  )
	{
		// 첨부파일 오류
		if( filepath.IsEmpty() ) {
			Log(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertDirectData() filepath error"));
			g_cs.Unlock();
			return -1;
		}

		query.Format(_T("select * from mmsinfo where uid='%s' and fileFullPath='%s'"), id, filepath);
		if(mysql_query(&_mysql, query))
		{ 
			Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertDirectData(), mysql_query error(%s)"),mysql_error(&_mysql));
			g_cs.Unlock();
			return -1;
		}

		if((_res = mysql_store_result(&_mysql)) == NULL)
		{ 
			Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertDirectData(), mysql_store_result error(%s)"),mysql_error(&_mysql));
			g_cs.Unlock();
			return -1;
		}

		if((_row = mysql_fetch_row(_res)) == NULL)
		{
			// 출력창에 디버깅 용으로 
			mysql_free_result(_res);
			g_cs.Unlock();
			return -1;
		}				

		mmsfile_id = _row[0];

		mysql_free_result(_res);
	}

	CString reserveTimeValue;
	if( toupper(isReserved) == 'Y' )
	{
		reserveTimeValue.Format(_T("\'%s\'"), reservedTime);
	}
	else
		reserveTimeValue = "NULL";

	query.Format(_T("insert into msgqueue(uid, sendNo, kind, callbackNo, receiveNo, subject, msg, mmsfile_id, registTime, isReserved, reservedTime) values('%s','%s',%d,'%s','%s','%s','%s',%s, NOW(), '%c', %s)"),
						id, send_number, msgtype, callbackNo, receiveNo, subject, msg, mmsfile_id, isReserved, reserveTimeValue );
	
	if(mysql_query(&_mysql, query))
	{ 
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::InsertDirectData(), mysql_query2 error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return -1;
	}

	g_cs.Unlock();

	return 1;
}

bool DBManager::DeleteReservedMsgQueue(CString idList)
{
	g_cs.Lock();

	CString query;
	query.Format(_T("delete from msgqueue where send_queue_id in (%s)"), idList);
	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::DeleteReservedMsgQueue(), mysql_query, delete error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	g_cs.Unlock();

	return true;
}

bool DBManager::ModifyState_REQUEST(CString idList)
{
	g_cs.Lock();

	CString query;
	//query.Format(_T("update msgqueue set state=1, sendTime = NOW() where send_queue_id in (%s)"), idList);

	query.Format(_T("update msgqueue set state=1, sendTime = NOW() where send_queue_id in (%s)"), idList);

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_REQUEST(), mysql_error(%s)"), mysql_error(&_mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_REQUEST(), query(%s)"), query);
		g_cs.Unlock();
		return false;
	}

	mysql_autocommit(&_mysql, false); // start transaction 

	//query.Format(_T("insert into msgresult(uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result) \
	//					(select uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result from msgqueue where send_queue_id=%s)"), kshot_number);
	query.Format(_T("insert into msgresult(send_queue_id,uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result, telecom) (select * from msgqueue where send_queue_id in (%s))"), idList);

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_REQUEST(), mysql_query, insert error(%s)"), mysql_error(&_mysql));

		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	query.Format(_T("delete from msgqueue where send_queue_id in (%s)"), idList);
	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_REQUEST(), mysql_query, delete error(%s)"), mysql_error(&_mysql));

		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 


	g_cs.Unlock();

	return true;
}

bool DBManager::ModifyState_REQUEST(CString id, CString send_number_list)
{
	g_cs.Lock();

	CString query;
	query.Format(_T("update msgqueue set state=1, sendTime = NOW() where uid='%s' and sendNo in (%s)"), id, send_number_list);

	if(mysql_query(&_mysql, query))
	{ 
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_REQUEST(CString id, CString send_number_list), mysql_query error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	g_cs.Unlock();

	return true;
}

BOOL DBManager::IsExistResponse(CString& responseList)
{
	_cs.Lock();

	if (_list_response.IsEmpty()) {
		_cs.Unlock();
		return FALSE;
	}
	
	responseList = _list_response;

	_cs.Unlock();

	return TRUE;
}

void DBManager::AddResponse(CString id)
{
	_cs.Lock();

	if (! _list_response.IsEmpty() ) 
		_list_response += ",";

	_list_response += id;

	_cs.Unlock();
}

void DBManager::ClearResponse()
{
	_cs.Lock();
	_list_response.Empty();
	_cs.Unlock();
}

//BOOL DBManager::IsExistResult(vector<LIST_RESULT>& resultList)
BOOL DBManager::IsExistResult()
{
	//_cs2.Lock();

	if (_vt_result_list.size() <= 0) {
		//_cs2.Unlock();
		return FALSE;
	}

	//resultList = _vt_result_list;

	// 결과가 있은지 찾고

	vector<LIST_RESULT*>::iterator it = _vt_result_list.begin();
	for (int i = 0; it != _vt_result_list.end(); it++)
	{
		LIST_RESULT* result_list = *it;
		if (result_list->_list_khost_number.IsEmpty() == FALSE)
		{
			//_cs2.Unlock();
			return TRUE;
		}
	}

	//_cs2.Unlock();

	return FALSE;
}

void	DBManager::AddResult(CString kshot_number, int result, int telecom)
{
	_cs2.Lock();

	//TRACE(_T("kshot_number:%s, result:%d, telecom:%d\n"), kshot_number, result, telecom);

	BOOL notFound = TRUE;

	vector<LIST_RESULT*>::iterator it = _vt_result_list.begin();
	for (int i = 0; it != _vt_result_list.end(); it++) 
	{
		LIST_RESULT* result_list = *it;
		if (result_list->_result == result && result_list->_telecom == telecom) 
		{
			notFound = FALSE;

			if (result_list->_list_khost_number.IsEmpty() == FALSE) {
				result_list->_list_khost_number += ",";
			}
			result_list->_list_khost_number += kshot_number;

			//TRACE(_T("list_kshot_number:%s\n"), result_list->_list_khost_number);
			break;
		}
	}

	if (notFound) {
		LIST_RESULT * result_list = new LIST_RESULT;
		result_list->_result = result;
		result_list->_telecom = telecom;
		result_list->_list_khost_number += kshot_number;

		_vt_result_list.push_back(result_list);
	}

	_cs2.Unlock();
}

void DBManager::ClearResult() 
{
	//_cs2.Lock();

	//_vt_result_list.clear();

	vector<LIST_RESULT*>::iterator it = _vt_result_list.begin();
	for (int i = 0; it != _vt_result_list.end(); it++)
	{
		LIST_RESULT* result_list = *it;
		result_list->_list_khost_number.Empty();
	}

	//_cs2.Unlock();
}


bool DBManager::ModifyState_RESPONSE(CString idlist)
{
	g_cs.Lock();

	CString query;
	//query.Format(_T("update msgqueue set state=2, responseTime = NOW() where send_queue_id in (%s)"), idlist);
	query.Format(_T("update msgresult set state=2, responseTime = NOW() where send_queue_id in (%s)"), idlist);

	if(mysql_query(&_mysql, query))
	{ 
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::ModifyState_RESPONSE(CString id), mysql_query error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	g_cs.Unlock();

	return true;
}

int DBManager::LoadNonResultNotify(CString uid, vector<ClientManager::SEND_RESULT_INFO*>& vtList)
{
	g_cs.Lock();

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::LoadNonResultNotify(), set names euckr error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	CString query;
	query.Format(_T("select send_queue_id, sendNo, receiveNo, resultTime, result from msgresult where uid = '%s' and notify_result = 0"), uid);

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::LoadNonResultNotify(), mysql_query error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return -1;
	}

	if ((_res = mysql_store_result(&_mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::LoadNonResultNotify(), mysql_store_result error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return -1;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		ClientManager::SEND_RESULT_INFO *info = new ClientManager::SEND_RESULT_INFO();
		info->kshot_number = _ttoi(_row[0]);
		info->send_number = _row[1];
		info->receiveNo = _row[2];
		info->done_time = _row[3];
		info->result = _ttoi(_row[4]);

		vtList.push_back(info);
	}

	mysql_free_result(_res);

	g_cs.Unlock();
}

bool DBManager::UpdateResultNotify(CString notifyList)
{
	g_cs.Lock();

	CString query;
	query.Format(_T("update msgresult set notify_result=1 where send_queue_id in (%s)"), notifyList);

	if (mysql_query(&_mysql, query))
	{
		// 쿼리 요청
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::UpdateResultNotify(), mysql_query error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	g_cs.Unlock();

	return true;
}

bool DBManager::MoveToResultTable(CString list_kshot_number, int result, int telecom)
{
	g_cs.Lock();

	if( mysql_query(&_mysql,"set names euckr") )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::MoveToResultTable(), error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	CString query;
	//query.Format(_T("update msgqueue set state=3, resultTime = NOW(), result = %d, telecom = %d where send_queue_id in (%s)"), result, telecom, list_kshot_number);
	query.Format(_T("update msgresult set state=3, resultTime = NOW(), result = %d, telecom = %d where send_queue_id in (%s)"), result, telecom, list_kshot_number);

	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::MoveToResultTable(), mysql_error(%s)"), mysql_error(&_mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::MoveToResultTable(), error query(%s)"), query);
		g_cs.Unlock();
		return false;
	}

	/*
	mysql_autocommit(&_mysql, false); // start transaction 

	//query.Format(_T("insert into msgresult(uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result) \
	//					(select uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result from msgqueue where send_queue_id=%s)"), kshot_number);
	query.Format(_T("insert into msgresult(send_queue_id,uid,sendNo,kind,line_id,callbackNo,receiveNo,subject,msg,mmsfile_id,isReserved,reservedTime,registTime,sendTime,responseTime,resultTime,state,result, telecom) (select * from msgqueue where send_queue_id in (%s))"), list_kshot_number);
	
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::MoveToResultTable(), mysql_query, insert error(%s)"),mysql_error(&_mysql));
		
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	query.Format(_T("delete from msgqueue where send_queue_id in (%s)"), list_kshot_number);
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::MoveToResultTable(), mysql_query, delete error(%s)"),mysql_error(&_mysql));

		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}
	
	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 
	*/

	g_cs.Unlock();

	return true;
}

// sendNo 얻기
bool DBManager::GetSendNo(CString kshotNumber, CString& uid, CString& sendNo, CString& receiveNo, int& msgtype)
{
	g_cs.Lock();

	bool retval = false;

	CString query;
	query.Format(_T("select uid, sendNo, kind, receiveNo from msgresult where send_queue_id=%s"), kshotNumber);

	// mysql_query == 0 이면 성공
	if (mysql_query(&_mysql, query) == 0)
	{
		if ((_res = mysql_store_result(&_mysql)) != NULL)
		{
			if ((_row = mysql_fetch_row(_res)) != NULL)
			{
				uid			= _row[0];
				sendNo	= _row[1];
				msgtype = _ttoi(_row[2]);
				receiveNo = _row[3];

				retval = true;
			}
			else
				Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::GetSendNo(), mysql_fetch_row empty"));

			mysql_free_result(_res);
		}
		else
			Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::GetSendNo(), mysql_store_result null"));
	}
	else
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::GetSendNo(), mysql_query(%s) error(%s)"), query, mysql_error(&_mysql));

	g_cs.Unlock();

	return retval;
}

// 실패에 대한 환불처리
bool DBManager::refundBalance(CString uid, CString pid, int msgType, float price)
{
	g_cs.Lock();

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::refundBalance(), error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	CString query;
	if( pid.IsEmpty() )
		query.Format(_T("update member set balance = balance + %f where uid = '%s'"), price, uid);
	else
		query.Format(_T("update member set balance = balance + %f where uid = '%s'"), price, pid);

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::refundBalance(), mysql_query error(%s)"), mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	g_cs.Unlock();

	return true;
}

BOOL DBManager::refundBatch()
{
	// 환불처리
	// 만료 대상을 찾는다.
	// Select를 순회하면서 uid, kind, price, 찾는다.

	MYSQL_RES*		res;
	MYSQL_ROW		row;

	CString msg;

	CString query;

	query.Format(_T("select uid, kind, count(*) AS count from msgresult where state=2 and result=-1  and sendTime < DATE_ADD(now(),INTERVAL -2 DAY) group by uid, kind"));

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::refundBatch(), mysql_query1, error(%s)"), mysql_error(&_mysql));
		return FALSE;
	}

	if ((res = mysql_store_result(&_mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::refundBatch(), mysql_store_result1, error(%s)"), mysql_error(&_mysql));
		return FALSE;
	}

	if ((row = mysql_fetch_row(res)) == NULL) {
		Logmsg(CMdlLog::LEVEL_DEBUG, _T("DBManager::refundBatch(), mysql_fetch_row1, empty"));
		return FALSE;
	}

	char uid[30] = { 0 };
	int kind = -1;
	int count = -1;

	do
	{
		memset(uid, 0x00, sizeof(uid));
		kind = -1;
		count = -1;

		if (row[0] != NULL)
			strcpy(uid, row[0]);

		if (row[1] != NULL)
			kind = _ttoi(row[1]);

		if (row[2] != NULL)
			count = _ttoi(row[2]);

		{
			CString query2;
			if (kind == 0)
				query2.Format(_T("update member set balance = balance+sms_price*%d where uid = '%s'"), count, uid);
			else if (kind == 1)
				query2.Format(_T("update member set balance = balance+lms_price*%d where uid = '%s'"), count, uid);
			else if (kind == 2)
				query2.Format(_T("update member set balance = balance+mms_price*%d where uid = '%s'"), count, uid);
			else if (kind == 3)
				query2.Format(_T("update member set balance = balance+fax_price*%d where uid = '%s'"), count, uid);

			if (mysql_query(&_mysql, query2))
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::refundBatch(), mysql_query2, error(%s)\n"), mysql_error(&_mysql));
				return FALSE;
			}
		}
	} while ((row = mysql_fetch_row(res)) != NULL);
}

BOOL DBManager::FailForExpireBatch()
{
	mysql_autocommit(&_mysql, false); // start transaction 


	// 환불처리
	if (refundBatch() == FALSE) {
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 
		return FALSE;
	}

	CString query;

	// 대기 만료된 것을 실패로 처리(999, 9999)
	query.Format(_T("update msgresult set state=3, 	result = CASE WHEN kind = 0 THEN 999 ELSE 9999 END where state=2 and result=-1 and sendTime < DATE_ADD(now(),INTERVAL -2 DAY)"));

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpireBatch(), update error(%s)"), mysql_error(&_mysql));
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 
		return FALSE;
	}

	/*
	query.Format(_T("insert into msgresult(send_queue_id, uid, sendNo, kind, line_id, callbackNo, receiveNo, subject, msg, mmsfile_id, isReserved, reservedTime, registTime, sendTime, responseTime, resultTime, state, result, telecom) (select * from msgqueue where state=3 and result=999)"));
	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FailForExpireBatch(), insert error(%s)"), mysql_error(&_mysql));
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 
		return FALSE;
	}

	query.Format(_T("delete from msgqueue where state=3 and result=999"));
	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FailForExpireBatch(), delete error(%s)"), mysql_error(&_mysql));

		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 

		return FALSE;
	}
	*/

	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 

	return TRUE;
}

BOOL DBManager::FailForExpire(int kshot_number, CString uid, CString pid, int msgtype, float price)
{
	mysql_autocommit(&_mysql, false); // start transaction 


	// 대기 만료된 것을 실패로 처리(999, 9999)
	CString query;
	if( msgtype == 0 )
		query.Format(_T("update msgresult set state=3, result = 999 where send_queue_id=%d"), kshot_number);
	else
		query.Format(_T("update msgresult set state=3, result = 9999 where send_queue_id=%d"), kshot_number);

	if (mysql_query(&_mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpire(), update error(%s)"), mysql_error(&_mysql));
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 
		return FALSE;
	}

	// 환불처리
	CString query2;

	//CString pricetype;
	//if (msgtype == 0)
	//	pricetype = "sms_pirce";
	//else if (msgtype == 1)
	//	pricetype = "lms_pirce";
	//else if (msgtype == 2)
	//	pricetype = "mms_pirce";

	if (pid.IsEmpty())
		query2.Format(_T("update member set balance = balance + %f where uid = '%s'"), price, uid);
	else
		query2.Format(_T("update member set balance = balance + %f where uid = '%s'"), price, pid);

	if (mysql_query(&_mysql, query2))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpire(), insert error(%s)"), mysql_error(&_mysql));
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction
		return FALSE;
	}

	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 

	return TRUE;
}

bool DBManager::FailForExpireSMS(CString kshot_number)
{
	g_cs.Lock();

	if( mysql_query(&_mysql,"set names euckr") )
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpireSMS(), error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	CString query;

	query.Format(_T("insert into msgresult (select * from msgqueue where send_queue_id=%s)"), kshot_number);
	
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpireSMS(), insert error(%s)"),mysql_error(&_mysql));
		g_cs.Unlock();
		return false;
	}

	mysql_autocommit(&_mysql, false); // start transaction 

	// 대기 만료된 것을 실패로 처리(99)
	query.Format(_T("update msgresult set state=3, result = %d where send_queue_id=%s"), 99, kshot_number);
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpireSMS(), update error(%s)"),mysql_error(&_mysql));
		
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();
		return false;
	}

	query.Format(_T("delete from msgqueue where send_queue_id=%s"), kshot_number);
	if(mysql_query(&_mysql, query))
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::FailForExpireSMS(), delete error(%s)"),mysql_error(&_mysql));
		
		mysql_rollback(&_mysql);
		mysql_autocommit(&_mysql, true); // end transaction 

		g_cs.Unlock();

		return false;
	}
	
	mysql_commit(&_mysql);
	mysql_autocommit(&_mysql, true); // end transaction 

	g_cs.Unlock();

	return true;
}

bool DBManager::checkNReconnect(MYSQL*mysql /*= NULL*/)
{
	g_cs.Lock();

	BOOL mysql_Of_DBManager = FALSE;

	if (mysql == NULL) {
		mysql = &_mysql;

		mysql_Of_DBManager = TRUE;
	}

	/*
	MYSQL_RES*		res;
	MYSQL_ROW		row;

	CString query;
	query.Format(_T("select * from admin"));
	
	if (mysql_query(mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() mysql_query(%s) error(%s)"), query, mysql_error(mysql));
		return false;
	}

	if ((res = mysql_store_result(mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() mysql_store_result error(%s)"), mysql_error(mysql));
		return false;
	}

	if ((row = mysql_fetch_row(_res)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() mysql_fetch_row empty(%s)"), mysql_error(mysql));
		return false;
	}
	*/

	unsigned int errorno = 0;

	if (mysql_ping(mysql) == 0) {
		TRACE(_T("DBManager::checkNReconnect() 정상동작 중)\n"));
		g_cs.Unlock();
		return true;
	}
	else {
		errorno = mysql_errno(mysql);
	}

	switch(errorno)
	{
		case CR_COMMANDS_OUT_OF_SYNC: 
			Log(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() error(명령이 불규칙한 순서로 실행됨)")); 
			break;
		case CR_SERVER_GONE_ERROR:
			Log(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() error(서버가 중단됨)"));
			break;
		case CR_UNKNOWN_ERROR:
			Log(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() error(알수 없는 에러)"));
			break;
		default:
			Log(CMdlLog::LEVEL_ERROR, _T("DBManager::checkNReconnect() error(알수 없는 에러)"));
	}

	if (errorno == CR_SERVER_GONE_ERROR)
	{
		while(Reconnect(mysql) == false )
			Sleep(1000);

		if (mysql_Of_DBManager)
			AlterAutoIncrement();

		g_cs.Unlock();
		return true;
	}
		
	g_cs.Unlock();

	return false;
}