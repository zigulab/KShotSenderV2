// HaomunAgent.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "HaomunAgent.h"
#include "DBManager.h"
#include "sendManager.h"

#include "./include/mysql/errmsg.h"


// HaomunAgent

HaomunAgent::HaomunAgent(DBManager*	dbManager)
{
	_dbManager = dbManager;
}

HaomunAgent::~HaomunAgent()
{
}


// HaomunAgent 멤버 함수

// HaomunAgent 멤버 함수
BOOL HaomunAgent::Init()
{
	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;

	int useagent = _ttoi(g_config.haomunAgent.useAgent);
	if (useagent)
	{
		_hRunDelegatorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hRunDelegatorEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);

		AfxBeginThread(AgentProc, this);
		AfxBeginThread(ResultWatchProc, this);
	}

	return TRUE;
}

BOOL HaomunAgent::UnInit()
{
	_bExitFlag = TRUE;

	int useagent = _ttoi(g_config.haomunAgent.useAgent);
	if (useagent) 
	{
		SetEvent(_hRunDelegatorEvent);
		while (!_bDoneThreadFlag)
			::Sleep(10);

		SetEvent(_hRunDelegatorEvent2);
		while (!_bDoneThreadFlag2)
			::Sleep(10);
	}

	return TRUE;
}

void HaomunAgent::SetNewSendNotify()
{
	while (_agent_state == AS_DOING)
		Sleep(10);

	SetEvent(_hRunDelegatorEvent);
}

void HaomunAgent::AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas)
{
	_cs2.Lock();

	//_vtOutAgentDatas = vtOutAgentDatas;
	_vtOutAgentDatas.insert(_vtOutAgentDatas.end(), vtDatas.begin(), vtDatas.end());

	_cs2.Unlock();
}

int	HaomunAgent::GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	_cs2.Lock();

	int count = _vtOutAgentDatas.size();

	vtDatas = _vtOutAgentDatas;

	_cs2.Unlock();

	return count;
}

void	HaomunAgent::DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	_cs2.Lock();

	//vector<SEND_QUEUE_INFO*>::iterator it = _vtOutAgentDatas.begin();
	//for (; it != _vtOutAgentDatas.end(); it++) {
	//	delete *it;
	//	
	//	vector<SEND_QUEUE_INFO*>::iterator it2 = vtDatas.begin();
	//	for (; it2; it2 != vtDatas.end(); it2++) {

	//	}
	//}
	//_vtOutAgentDatas.clear();


	int size = vtDatas.size();

	vector<SEND_QUEUE_INFO*>::iterator it = _vtOutAgentDatas.begin();

	vector<SEND_QUEUE_INFO*>::iterator it2 = it;
	advance(it2, size);

	_vtOutAgentDatas.erase(it, it2);

	vector<SEND_QUEUE_INFO*>::iterator pos = vtDatas.begin();
	for (; pos != vtDatas.end(); pos++) {
		delete *pos;
	}

	_cs2.Unlock();
}

void HaomunAgent::SetKshotNumber(int number)
{
	_cs.Lock();
	_vtKshotNumber.push_back(number);
	_cs.Unlock();
}

CString HaomunAgent::MakeKshotNumberList()
{
	CString List;

	_cs.Lock();
	vector<int>::iterator it = _vtKshotNumber.begin();
	for (; it != _vtKshotNumber.end(); it++) {
		CString str;
		str.Format(_T("'%d'"), *it);

		List += str;
		List += ",";
	}
	_cs.Unlock();

	if (!List.IsEmpty())
		List.Delete(List.GetLength() - 1);

	return List;
}

CString HaomunAgent::MakeJobList()
{
	CString List;

	_cs.Lock();
	vector<int>::iterator it = _vtKshotNumber.begin();
	for (; it != _vtKshotNumber.end(); it++) 
	{
		map<int, LONG64>::iterator it2 = _mpKshotNumber.find(*it);
		if (it2 != _mpKshotNumber.end()) 
		{
			LONG64 jobid = it2->second;

			CString str;
			str.Format(_T("%lld"), jobid);

			List += str;
			List += ",";
		}

	}
	_cs.Unlock();

	if (!List.IsEmpty()) {
		List.Delete(List.GetLength() - 1);
	}

	return List;
}

void HaomunAgent::DeleteKshotNumber(vector<int>list)
{
	_cs.Lock();
	vector<int>::iterator it = list.begin();
	for (; it != list.end(); it++)
	{
		vector<int>::iterator it2 = _vtKshotNumber.begin();
		for (; it2 != _vtKshotNumber.end(); it2++) {
			if (*it2 == *it)
			{
				_vtKshotNumber.erase(it2);
				break;
			}
		}

		map<int, LONG64>::iterator pos = _mpKshotNumber.begin();
		for(; pos != _mpKshotNumber.end(); pos++)
		{
			if (pos->first == *it) {
				_mpKshotNumber.erase(pos);
				break;
			}
		}
	}
	_cs.Unlock();
}

// 그 현재 fseqenunce최고값 구한다.
int HaomunAgent::GetResultRow(MYSQL& mysql, CString kshotNumberList)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select job_id, reserved1 from SDK_MMS_REPORT where job_id != 0 and (succ_count = 1 or fail_count = 1 ) and reserved1 in (%s)"), kshotNumberList);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		// 테이블이 존재하지 않음. ( 일일 처음 발송일 때 )
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResultRow(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResultRow(), query(%s)"), query);
		return -1;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResultRow(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return -1;
	}

	int row = 0;
	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		LONG64 jobid = _ttoi64(_row[0]);
		int kshot_number = _ttoi(_row[1]);

		_mpKshotNumber[kshot_number] = jobid;

		row++;
	}

	mysql_free_result(_res);

	return row;
}

// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
void HaomunAgent::GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString jobList)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select job_id, tcs_result, report_res_date, mobile_info from SDK_MMS_REPORT_DETAIL where job_id in (%s)"), jobList);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResult(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResult(), query(%s)"), query);
		return;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::GetResult(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		LONG64 jobid = _ttoi64(_row[0]);

		map<int, LONG64>::iterator it = _mpKshotNumber.begin();
		for (; it != _mpKshotNumber.end(); it++)
		{
			if (it->second == jobid)
			{
				rp.kshot_number = it->first;
				break;
			}
		}

		rp.result = _ttoi(_row[1]);
		strcpy(rp.resultTime, _row[2]);
		rp.telecom = _ttoi(_row[3]);

		vtResultPack.push_back(rp);
	}

	mysql_free_result(_res);
}

// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
void HaomunAgent::SendAgentResult(vector<resultPack>& vtResultPack)
{
	//void SendManager::PushResult(char* kshot_number, int result, char* time, int telecom)

	vector<resultPack>::iterator it = vtResultPack.begin();
	for (; it != vtResultPack.end(); it++) {

		resultPack rp = *it;

		CString strNumber;
		strNumber.Format(_T("%d"), rp.kshot_number);

		CString strTime;
		strTime.Format(_T("%s"), rp.resultTime);

		// 1: SKT, 2:KT, 3:LG, 4:AHNN, 5:DACOM, 6:SK Broadband, 7 : KTF, 8 : LGT, 0 : Unkwon

		int nTelecom;

		if (rp.telecom == 1)
			nTelecom = 1;
		else if (rp.telecom == 2)
			nTelecom = 7;
		else if (rp.telecom == 3)
			nTelecom = 8;
		else
			nTelecom = 0;

		g_sendManager->PushResult(strNumber.GetBuffer(), rp.result, strTime.GetBuffer(), nTelecom);
	}
}

bool HaomunAgent::processResult(MYSQL& mysql, CString kshotNumberList)
{
	int newRow = GetResultRow(mysql, kshotNumberList);

	// 테이블 생성되지 않았을 때( 일일 처음 발송일 때 )
	if (newRow <= 0)
		return false;

	// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
	CString jobList = MakeJobList();

	if (jobList.IsEmpty())
		return false;

	vector<resultPack> vt;
	GetResult(mysql, vt, jobList);

	// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
	if (!vt.empty())
		SendAgentResult(vt);

	vector<int> list;
	vector<resultPack>::iterator it = vt.begin();
	for (it; it != vt.end(); it++) {
		resultPack rp = *it;
		list.push_back(rp.kshot_number);
	}

	DeleteKshotNumber(list);

	return true;
}


bool HaomunAgent::SetKNumberWithReload(MYSQL& mysql)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select kshot_number from tblmessage where fsenddate > DATE_ADD(now(),INTERVAL -2 DAY)"));

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::SetKNumberWithReload(), mysql_query(%s) error(%s)"), query, mysql_error(&mysql));
		return false;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::SetKNumberWithReload(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return false;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		int kshot_number;

		kshot_number = _ttoi(_row[0]);

		SetKshotNumber(kshot_number);
	}

	mysql_free_result(_res);

	return true;
}


UINT HaomunAgent::AgentProc(LPVOID lpParameter)
{
	HaomunAgent* delegator = (HaomunAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.haomunAgent.server;
	CString _id = g_config.haomunAgent.id;
	CString _pwd = g_config.haomunAgent.password;
	CString _dbname = g_config.haomunAgent.dbname;
	CString _dbport = g_config.haomunAgent.dbport;

	Log(CMdlLog::LEVEL_EVENT, _T("HaomunAgent::AgentProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("HaomunAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("HaomunAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::AgentProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	// 재시작후 kshot_number 설정
	//if (delegator->SetKNumberWithReload(_mysql) == false)
	//	return false;

	while (1)
	{
		if (delegator->_bExitFlag)
			break;

		// 30분( 60*60*0.5 ) 간격
		int timeout_inerval = (60 * 60 * 0.5) * 1000;  // millisec

		delegator->_agent_state = AS_WAITING;

		DWORD ret = WaitForSingleObject(delegator->_hRunDelegatorEvent, timeout_inerval);  // INFINITE;
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::AgentProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::AgentProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("HaomunAgent::AgentProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
						Log(CMdlLog::LEVEL_ERROR, msg);
						return false;
					}
				}
			}
			continue;
		}

		delegator->_agent_state = AS_DOING;

		vector<SEND_QUEUE_INFO*> vtAgentQueue;

		int curCount = 0;

		// 저장된 버퍼을 읽어 들임
		curCount = delegator->GetOutAgentData(vtAgentQueue);

		if (curCount < 1)
			continue;

		CString query;
		query.Format(_T("insert into SDK_MMS_SEND( USER_ID, SUBJECT, NOW_DATE, SEND_DATE, DEST_COUNT, DEST_INFO, MSG_TYPE, MMS_MSG, CONTENT_COUNT, CONTENT_DATA, CALLBACK, RESERVED1) values"));

		int loopcnt = 0;

		vector<SEND_QUEUE_INFO*>::iterator it = vtAgentQueue.begin();
		for (; it != vtAgentQueue.end(); it++)
		{
			SEND_QUEUE_INFO* info = *it;

			CString OneRowValue;
			CString reserveTimeValue;

			if (toupper(info->isReserved) == 'Y')
			{
				reserveTimeValue.Format(_T("\'%s\'"), info->reservedTime);
			}
			else
				reserveTimeValue = "NULL";

			// 문장내 ' =>\' 변경
			CString smsMessage = info->msg;
			smsMessage.Replace("'", "\\'");

			CString smsSubject = info->subject;
			if (info->msgtype > 0)
				smsSubject.Replace("'", "\\'");

			CString strReceive;
			strReceive.Format(_T("mms^%s"), info->receive);

			CString filePath;
			filePath.Format(_T("http://%s:%s%s^1^0"), g_config.kshot.mmsServerIP, g_config.kshot.mmsServerPort, info->filepath);

			//OneRowValue.Format(_T(" ('%s', '%s', DATE_FORMAT(now(), '\%Y\%m\%d\%H\%i'), DATE_FORMAT(now(), '\%Y\%m\%d\%H\%i'), 1, \
			//									'%s',0,'%s',1, \
			//									'%s', '%s', '%d')"),
			//	info->uid, smsSubject, 
			//	strReceive, smsMessage,
			//	filePath, info->callback, info->kshot_number
			//);

			OneRowValue.Format(_T(" ('%s', '%s', %s, %s, 1, '%s',0,'%s',1, '%s', '%s', '%d')"),
				info->uid, smsSubject, _T("DATE_FORMAT(now(), '%Y%m%d%H%i')"), _T("DATE_FORMAT(now(), '%Y%m%d%H%i')"),
				strReceive, smsMessage,
				filePath, info->callback, info->kshot_number
			);

			query += OneRowValue;
			query += ",";

			delegator->SetKshotNumber(info->kshot_number);

			loopcnt++;
		}
		query.Delete(query.GetLength() - 1);

		if (mysql_query(&_mysql, query))
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::AgentProc(), mysql_error(%s)"), mysql_error(&_mysql));
			query = query.Left(4000);
			Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::AgentProc(), query (%s)"), query);
		}

		CString logmsg;
		logmsg.Format(_T("HaomunAgent::AgentProc(), [msg] 외부 Agent모듈로 발송 전달(count:%d)"), loopcnt);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		delegator->DeleteOutAgentData(vtAgentQueue);
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("HaomunAgent::AgentProc(), 종료됨."));

	delegator->_bDoneThreadFlag = TRUE;

	return 0;
}


UINT HaomunAgent::ResultWatchProc(LPVOID lpParameter)
{
	HaomunAgent* delegator = (HaomunAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.haomunAgent.server;
	CString _id = g_config.haomunAgent.id;
	CString _pwd = g_config.haomunAgent.password;
	CString _dbname = g_config.haomunAgent.dbname;
	CString _dbport = g_config.haomunAgent.dbport;

	Log(CMdlLog::LEVEL_EVENT, _T("HaomunAgent::ResultWatchProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("HaomunAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("HaomunAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::ResultWatchProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	// 현재 Seq
	// int	curRow = delegator->GetResultRow(_mysql, _T("' '"));

	// 20분 간격으로 DB연결 확인
	int idleTime = 0;
	int checkConnectionInterval =  (1 * 20 * 60) * 1000;  // millisec

	while (1)
	{
		if (delegator->_bExitFlag)
			break;

		// 2초 간격으로 확인
		int timeout_inerval = 2 * 1000;  // millisec

		DWORD ret = WaitForSingleObject(delegator->_hRunDelegatorEvent2, timeout_inerval);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::ResultWatchProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT && idleTime > checkConnectionInterval)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("HaomunAgent::ResultWatchProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("HaomunAgent::ResultWatchProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
						Log(CMdlLog::LEVEL_ERROR, msg);
						return false;
					}
				}
			}
			
			idleTime = 0;

			continue;
		}
		
		// 그 일자에 해당하는 테이블의 row를 구한다.
		CString kshotNumberList = delegator->MakeKshotNumberList();

		if (kshotNumberList.IsEmpty()) {
			idleTime += timeout_inerval;

			continue;
		}

		if( delegator->processResult(_mysql, kshotNumberList) == true )
			idleTime = 0;
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("HaomunAgent::ResultWatchProc(), 종료됨."));

	delegator->_bDoneThreadFlag2 = TRUE;

	return 0;
}