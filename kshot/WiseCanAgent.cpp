// WiseCanAgent.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "WiseCanAgent.h"
#include "DBManager.h"
#include "sendManager.h"

#include "./include/mysql/errmsg.h"

// WiseCanAgent


WiseCanAgent::WiseCanAgent(DBManager*	dbManager)
{
	_dbManager = dbManager;
}

WiseCanAgent::~WiseCanAgent()
{
}


// WiseCanAgent 멤버 함수
BOOL WiseCanAgent::Init()
{
	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;

	int useagent = _ttoi(g_config.wisecanAgent.useAgent);
	if (useagent) 
	{
		_hRunDelegatorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hRunDelegatorEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);
	
		AfxBeginThread(AgentProc, this);
		AfxBeginThread(ResultWatchProc, this);
	}

	return TRUE;
}

BOOL WiseCanAgent::UnInit()
{
	_bExitFlag = TRUE;

	int useagent = _ttoi(g_config.wisecanAgent.useAgent);
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

void WiseCanAgent::SetNewSendNotify()
{
	while (_agent_state == AS_DOING)
		Sleep(10);

	SetEvent(_hRunDelegatorEvent);
}

void WiseCanAgent::AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas)
{
	_cs2.Lock();

	//_vtOutAgentDatas = vtOutAgentDatas;
	_vtOutAgentDatas.insert(_vtOutAgentDatas.end(), vtDatas.begin(), vtDatas.end());

	_cs2.Unlock();
}

int	WiseCanAgent::GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	_cs2.Lock();

	int count = _vtOutAgentDatas.size();

	vtDatas = _vtOutAgentDatas;

	_cs2.Unlock();

	return count;
}

void	WiseCanAgent::DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
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

void WiseCanAgent::SetKshotNumber(int number)
{
	_cs.Lock();
	_vtKshotNumber.push_back(number);
	_cs.Unlock();
}

CString WiseCanAgent::MakeKshotNumberList()
{
	CString List;

	_cs.Lock();
	vector<int>::iterator it = _vtKshotNumber.begin();
	for (; it != _vtKshotNumber.end(); it++) {
		CString str;
		str.Format(_T("%d"), *it);

		List += str;
		List += ",";
	}
	_cs.Unlock();

	if (!List.IsEmpty()) {
		List.Delete(List.GetLength() - 1);
	}

	return List;
}

CString WiseCanAgent::MakeJobList()
{
	CString List;

	_cs.Lock();
	vector<int>::iterator it = _vtKshotNumber.begin();
	for (; it != _vtKshotNumber.end(); it++)
	{
		map<int, int>::iterator it2 = _mpKshotNumber.find(*it);
		if (it2 != _mpKshotNumber.end())
		{
			int msgkey = it2->second;

			CString str;
			str.Format(_T("%d"), msgkey);

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

void WiseCanAgent::DeleteKshotNumber(vector<int>list)
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

		map<int, int>::iterator pos = _mpKshotNumber.begin();
		for (; pos != _mpKshotNumber.end(); pos++)
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
int WiseCanAgent::GetResultRow(MYSQL& mysql, CString kshotNumberList)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select fsequence, kshot_number from %s where kshot_number in (%s)"), _curResultTable, kshotNumberList);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		// 테이블이 존재하지 않음. ( 일일 처음 발송일 때 )
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::GetResultRow(), mysql_query(%s) error(%s)"), query, mysql_error(&mysql));
		return -1;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::GetResultRow(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return -1;
	}

	int row = 0;
	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		int msgkey = _ttoi(_row[0]);
		int kshot_number = _ttoi(_row[1]);

		_mpKshotNumber[kshot_number] = msgkey;

		row++;
	}

	mysql_free_result(_res);

	return row;
}

// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
void WiseCanAgent::GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString joblist)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	//query.Format(_T("select fsequence, kshot_number, frsltstat, fmobilecomp,  frsltdate from %s where fsequence > %d"), _curResultTable, prevMax);
	query.Format(_T("select fsequence, kshot_number, frsltstat, fmobilecomp,  frsltdate from %s where fsequence in (%s)"), _curResultTable, joblist);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::GetResult(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::GetResult(), query(%s)"), query);
		return;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::GetResult(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		rp.seq =  _ttoi(_row[0]);
		rp.kshot_number = _ttoi(_row[1]);

		rp.result	= _ttoi(_row[2]);
		strcpy(rp.telecom, _row[3]);
		strcpy(rp.resultTime, _row[4]);

		vtResultPack.push_back(rp);
	}

	mysql_free_result(_res);
}

// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
void WiseCanAgent::SendAgentResult(vector<resultPack>& vtResultPack)
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

		if (stricmp(rp.telecom, "SKT") == 0 )
			nTelecom = 1;
		else if (stricmp(rp.telecom, "KT") == 0)
			nTelecom = 2;
		else if (stricmp(rp.telecom, "LG") == 0)
			nTelecom = 3;
		else if (stricmp(rp.telecom, "AHNN") == 0)
			nTelecom = 4;
		else if (stricmp(rp.telecom, "DACOM") == 0)
			nTelecom = 5;
		else if (stricmp(rp.telecom, "SK Broadand") == 0)
			nTelecom = 6;
		else if (stricmp(rp.telecom, "KTF") == 0)
			nTelecom = 7;
		else if (stricmp(rp.telecom, "LGT") == 0)
			nTelecom = 8;
		else
			nTelecom = 0;

		// WiseCan에서 6 은 전송성공
		if (rp.result == 6)
			rp.result = 0;

		g_sendManager->PushResult(strNumber.GetBuffer(), rp.result, strTime.GetBuffer(), nTelecom);
	}
}

bool WiseCanAgent::processResult(MYSQL& mysql, CString kshotNumberList)
{
	// 현재일자를 구한다.
	CTime    CurTime = CTime::GetCurrentTime();
	CString  formatTime = CurTime.Format("tmsglog_%Y%m");

	_curResultTable = formatTime;

	// 그 일자에 해당하는 테이블의 row를 구한다.
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


bool WiseCanAgent::SetKNumberWithReload(MYSQL& mysql)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select kshot_number from tblmessage where fsenddate > DATE_ADD(now(),INTERVAL -2 DAY)"));

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::SetKNumberWithReload(), mysql_query(%s) error(%s)"), query, mysql_error(&mysql));
		return false;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::SetKNumberWithReload(), mysql_store_result error(%s)"), mysql_error(&mysql));
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


UINT WiseCanAgent::AgentProc(LPVOID lpParameter)
{
	WiseCanAgent* delegator = (WiseCanAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.wisecanAgent.dbName;
	CString _dbport = g_config.db.dbport;

	Log(CMdlLog::LEVEL_EVENT, _T("WiseCanAgent::AgentProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("WiseCanAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("WiseCanAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::AgentProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	// 재시작후 kshot_number 설정
	//if (delegator->SetKNumberWithReload(_mysql) == false)
	//	return false;

	while (1)
	{
		if (delegator->_bExitFlag)
			break;

		// 8시간( 60*60*8 ) 간격
		int timeout_inerval = (60 * 60 * 8) * 1000;  // millisec

		delegator->_agent_state = AS_WAITING;

		DWORD ret = WaitForSingleObject(delegator->_hRunDelegatorEvent, timeout_inerval);  // INFINITE;
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::AgentProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::AgentProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("WiseCanAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
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
		query.Format(_T("insert into TBLMESSAGE(kshot_number,fuserid,fsenddate, fdestine, fcallback ,ftext) values"));

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

			OneRowValue.Format(_T(" (%d,'%s',NOW(),'%s','%s','%s')"),
				info->kshot_number,
				info->uid,
				info->receive,
				info->callback,
				smsMessage.GetBuffer()
			);

			query += OneRowValue;
			query += ",";

			delegator->SetKshotNumber(info->kshot_number);

			loopcnt++;
		}
		query.Delete(query.GetLength() - 1);

		if (mysql_query(&_mysql, query))
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::AgentProc(), mysql_error(%s)"), mysql_error(&_mysql));
			query = query.Left(4000);
			Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::AgentProc(), query (%s)"), query);
		}

		CString logmsg;
		logmsg.Format(_T("WiseCanAgent::AgentProc(), [msg] 외부 Agent모듈로 발송 전달(count:%d)"), loopcnt);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		delegator->DeleteOutAgentData(vtAgentQueue);
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("WiseCanAgent::AgentProc(), 종료됨."));

	delegator->_bDoneThreadFlag = TRUE;

	return 0;
}


UINT WiseCanAgent::ResultWatchProc(LPVOID lpParameter)
{
	WiseCanAgent* delegator = (WiseCanAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.wisecanAgent.dbName;
	CString _dbport = g_config.db.dbport;


	Log(CMdlLog::LEVEL_EVENT, _T("WiseCanAgent::ResultWatchProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("WiseCanAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("WiseCanAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::ResultWatchProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	// 현재 Seq
	//int	prevRow = delegator->GetResultRow(_mysql);

	// 20분 간격으로 DB연결 확인
	int idleTime = 0;
	int checkConnectionInterval = (1 * 20 * 60) * 1000;  // millisec

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
			Log(CMdlLog::LEVEL_ERROR, _T("WiseCanAgent::ResultWatchProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT && idleTime > checkConnectionInterval)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::ResultWatchProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("WiseCanAgent::ResultWatchProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
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

		if (delegator->processResult(_mysql, kshotNumberList) == true)
			idleTime = 0;

	}

	Log(CMdlLog::LEVEL_DEBUG, _T("WiseCanAgent::ResultWatchProc(), 종료됨."));

	delegator->_bDoneThreadFlag2 = TRUE;

	return 0;
}