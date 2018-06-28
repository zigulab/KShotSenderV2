// FaxAgent.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "FaxAgent.h"
#include "DBManager.h"
#include "sendManager.h"

#include "./include/mysql/errmsg.h"

// FaxAgent

CCriticalSection	g_csFaxAgentData;


FaxAgent::FaxAgent(DBManager*	dbManager)
{
	_dbManager = dbManager;
}

FaxAgent::~FaxAgent()
{
}


// FaxAgent 멤버 함수
BOOL FaxAgent::Init()
{
	// 현재일자를 구한다.
	CTime    CurTime = CTime::GetCurrentTime();
	CString  formatTime = CurTime.Format("tmsglog_%Y%m");

	_curResultTable = formatTime;

	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;

	int useagent = _ttoi(g_config.xpediteAgent.useAgent);
	if (useagent)
	{
		_hRunDelegatorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hRunDelegatorEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);

		AfxBeginThread(AgentProc, this);
		AfxBeginThread(ResultWatchProc, this);
	}

	return TRUE;
}

BOOL FaxAgent::UnInit()
{
	_bExitFlag = TRUE;

	int useFaxAgent = _ttoi(g_config.xpediteAgent.useAgent);
	if (useFaxAgent) 
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

void FaxAgent::SetNewSendNotify()
{
	while (_agent_state == AS_DOING)
		Sleep(10);

	SetEvent(_hRunDelegatorEvent);
}

void FaxAgent::AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas)
{
	g_csFaxAgentData.Lock();

	//_vtOutAgentDatas = vtOutAgentDatas;
	_vtOutAgentDatas.insert(_vtOutAgentDatas.end(), vtDatas.begin(), vtDatas.end());

	g_csFaxAgentData.Unlock();
}

int	FaxAgent::GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	g_csFaxAgentData.Lock();

	int count = _vtOutAgentDatas.size();

	vtDatas = _vtOutAgentDatas;

	g_csFaxAgentData.Unlock();

	return count;
}

void	FaxAgent::DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	g_csFaxAgentData.Lock();

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

	g_csFaxAgentData.Unlock();
}

void FaxAgent::SetKshotNumber(int number)
{
	_cs.Lock();
	_vtKshotNumber.push_back(number);
	_cs.Unlock();
}

CString FaxAgent::MakeKshotNumberList()
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

void FaxAgent::DeleteKshotNumber(vector<int>list)
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
	}
	_cs.Unlock();
}

// 이전 개수와 다르면 새로운 결과가 있는것으로 판단하고 결과를 읽어들인다.
void FaxAgent::GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString kshotNumberList)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	//query.Format(_T("select ResultCode, DoneDate, kshot_number from PGi_FaxList where kshot_number in (%s)"), kshotNumberList);
	query.Format(_T("select a.ResultCode, a.DoneDate, a.kshot_number, b.page from PGi_FaxList as a inner join pgi_faxtran as b on a.FaxTranSEQ = b.seq where kshot_number in (%s)"), kshotNumberList);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::GetResult(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::GetResult(), query(%s)"), query);
		g_cs.Unlock();
		return;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::GetResult(), mysql_store_result error(%s)"), mysql_error(&mysql));
		g_cs.Unlock();
		return;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		if (_row[0] != NULL)
			rp.result = _ttoi(_row[0]);
		else
			rp.result = -1;

		if (_row[1] != NULL)
			strcpy(rp.resultTime, _row[1]);

		rp.kshot_number = _ttoi(_row[2]);

		if(_row[3] != NULL)
			rp.telecom = _ttoi(_row[3]);

		if( rp.result != -1 )
			vtResultPack.push_back(rp);
	}

	mysql_free_result(_res);
}

void FaxAgent::failProcessBySystemFault()
{

}

// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
void FaxAgent::SendAgentResult(vector<resultPack>& vtResultPack)
{
	//void SendManager::PushResult(char* kshot_number, int result, char* time, int telecom)

	vector<resultPack>::iterator it = vtResultPack.begin();
	for (; it != vtResultPack.end(); it++) {

		resultPack rp = *it;

		CString strNumber;
		strNumber.Format(_T("%d"), rp.kshot_number);

		CString strTime;
		strTime.Format(_T("%s"), rp.resultTime);

		/*
		// 1: SKT, 2:KT, 3:LG, 4:AHNN, 5:DACOM, 6:SK Broadband, 7 : KTF, 8 : LGT, 0 : Unkwon
		int nTelecom;

		if (stricmp(rp.telecom, "SKT") == 0)
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
		*/

		g_sendManager->PushResult(strNumber.GetBuffer(), rp.result, strTime.GetBuffer(), rp.telecom);
	}
}

BOOL FaxAgent::processResult(MYSQL& mysql)
{
	CString kshotNumberList = MakeKshotNumberList();
	if (kshotNumberList.IsEmpty())
		return FALSE;

	vector<resultPack> vt;
	GetResult(mysql, vt, kshotNumberList);

	// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
	if (vt.empty()) return FALSE;

	SendAgentResult(vt);

	vector<int> list;
	vector<resultPack>::iterator it = vt.begin();
	for (it; it != vt.end(); it++) {
		resultPack rp = *it;
		list.push_back(rp.kshot_number);
	}

	DeleteKshotNumber(list);

	return TRUE;
}

int FaxAgent::ConvertNRegTrans(MYSQL& mysql, CString faxFilePath, CString faxUserId, CString faxPwd, CString filepath, SEND_QUEUE_INFO* info)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	//vector<SEND_QUEUE_INFO*>::iterator it = vtAgentQueue.begin();
	//SEND_QUEUE_INFO* info = *it;

	//CString filepath = info->filepath;

	// 경로에서 파일명만 추출
	int pos = filepath.ReverseFind('/');
	CString onlyFilePath = filepath.Mid(pos + 1, filepath.GetLength() - pos);

	CString realFilePath;
	realFilePath.Format(_T("%s\\%s\\%s"), faxFilePath, info->uid, onlyFilePath);

	// 경로의 쿼리적용을 위한 문자 변환
	realFilePath.Replace("\\", "\\\\");

	CString query;
	query.Format(_T("INSERT INTO PGi_FaxConvert(UserID, Req_File, Status ) VALUES ('%s', '%s', 1 )"), faxUserId, realFilePath);
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), INSERT INTO PGi_FaxConvert error(%s)"), mysql_error(&mysql));
		return -1;
	}

	query.Format(_T("SELECT seq, status, res_result FROM PGi_FaxConvert order by seq desc limit 1"));

	int convert_seq = 0;
	int status = 0;
	int res_result = 0;

	int trycnt = 0;
	do
	{
		if (trycnt > 0)
			Sleep(1000);
		else
		{
			// 5번까지만 시도
			if (trycnt >= 5)
				break;
		}

		if (mysql_query(&mysql, query) != 0)
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), do-while loop error(%s)"), mysql_error(&mysql));
			break;
		}

		if ((_res = mysql_store_result(&mysql)) == NULL)
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), do-while loop, result error(%s)"), mysql_error(&mysql));
			break;
		}

		if ((_row = mysql_fetch_row(_res)) != NULL)
		{
			//TRACE("mysql에 받은 값입니다 : no:%s uid:%s name:%s\n", _row[0], _row[2], _row[4]);
			if (_row[0] != NULL)
				convert_seq = _ttoi(_row[0]);

			if (_row[1] != NULL)
				status = _ttoi(_row[1]);

			if (_row[2] != NULL)
				res_result = _ttoi(_row[1]);
		}
		mysql_free_result(_res);

		trycnt++;

	} while (status != 3);

	// 실패 처리
	if (status != 3)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), converting occur? delay? error(%s)"), mysql_error(&mysql));
		return -1;
	}

	query.Format(_T("insert into PGi_FaxTran ( UserID, UserPW, Subject, FaxConvert_SEQ, SendDate ) values('%s', '%s', '%s', %d, NOW())"), faxUserId, faxPwd, info->subject, convert_seq);
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), insert into PGi_FaxTran error(%s)"), mysql_error(&mysql));
		return -1;
	}

	int trans_seq = 0;
	query.Format(_T("SELECT seq FROM PGi_FaxTran order by seq desc limit 1"));
	if (mysql_query(&mysql, query) != 0)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), SELECT seq FROM PGi_FaxTran error(%s)"), mysql_error(&mysql));
		return -1;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ConvertNRegTrans(), SELECT PGi_FaxTran, result error(%s)"), mysql_error(&mysql));
		return -1;
	}

	if ((_row = mysql_fetch_row(_res)) != NULL)
	{
		if (_row[0] != NULL)
			trans_seq = _ttoi(_row[0]);
	}
	mysql_free_result(_res);

	return trans_seq;
}

void FaxAgent::SetStatusIntoSend(MYSQL& mysql, CString query, int loopcnt, int trans_seq)
{
	query.Delete(query.GetLength() - 1);

	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::SetStatusIntoSend(), insert into PGi_FaxList error(%s)"), mysql_error(&mysql));
		return;
	}

	CString logmsg;
	logmsg.Format(_T("FaxAgent::SetStatusIntoSend(), [msg] fax Agent모듈로 발송 전달(count:%d)"), loopcnt);
	Log(CMdlLog::LEVEL_EVENT, logmsg);

	query.Format(_T("UPDATE PGi_FaxTran SET SendStatus=1 WHERE SEQ=%d"), trans_seq);
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::SetStatusIntoSend(), UPDATE PGi_FaxTran SET SendStatus=1 error(%s)"), mysql_error(&mysql));
	}
}

UINT FaxAgent::AgentProc(LPVOID lpParameter)
{
	FaxAgent* delegator = (FaxAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.xpediteAgent.dbName;
	CString _dbport = g_config.db.dbport;

	CString _faxFilePath = g_config.xpediteAgent.faxFilePath;
	CString _faxUserId = g_config.xpediteAgent.userId;
	CString _faxPwd = g_config.xpediteAgent.pwd;

	Log(CMdlLog::LEVEL_EVENT, _T("FaxAgent::AgentProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("FaxAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("FaxAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::AgentProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	while (1)
	{
		delegator->_agent_state = AS_WAITING;

		// 8시간( 60*60*8 ) 간격
		int timeout_inerval = (60 * 60 * 8) * 1000;  // millisec

		DWORD ret = WaitForSingleObject(delegator->_hRunDelegatorEvent, timeout_inerval);  // INFINITE
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("FaxAgent::AgentProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT) 
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::AgentProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("FaxAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
						Log(CMdlLog::LEVEL_ERROR, msg);
						return false;
					}
				}
			}
			continue;
		}

		delegator->_agent_state = AS_DOING;

		vector<SEND_QUEUE_INFO*> vtAgentQueue;

		// 저장된 버퍼을 읽어 들임
		int curCount = delegator->GetOutAgentData(vtAgentQueue);
		
		if (curCount < 1)
			continue;

		CString query;
		query.Format(_T("insert into PGi_FaxList ( kshot_number, UserID, FaxTranSEQ, XQN, REF, ADDR) values"));

		int trans_seq = 0;
		int loopcnt = 0;

		CString prevFilePath;

		vector<SEND_QUEUE_INFO*>::iterator it = vtAgentQueue.begin();
		for (; it != vtAgentQueue.end(); it++)
		{
			SEND_QUEUE_INFO* info = *it;

			if (prevFilePath.Compare(info->filepath) != 0) 
			{
				if (prevFilePath.IsEmpty() == FALSE)
				{
					delegator->SetStatusIntoSend(_mysql, query, loopcnt, trans_seq);
					query.Format(_T("insert into PGi_FaxList ( kshot_number, UserID, FaxTranSEQ, XQN, REF, ADDR) values"));
					loopcnt = 0;
				}

				trans_seq = delegator->ConvertNRegTrans(_mysql, _faxFilePath, _faxUserId, _faxPwd, info->filepath, info);

				if (trans_seq == -1)
					break;

				prevFilePath = info->filepath;
			}

			CString OneRowValue;
			CString reserveTimeValue;

			/*
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
			*/

			OneRowValue.Format(_T(" (%d,'%s',%d,%d,'%s','%s')"),
				info->kshot_number,
				_faxUserId,
				trans_seq,
				loopcnt+1,
				"케이피모바일",
				info->receive
			);

			query += OneRowValue;
			query += ",";

			delegator->SetKshotNumber(info->kshot_number);

			loopcnt++;
		}

		if( loopcnt > 0 )
			delegator->SetStatusIntoSend(_mysql, query, loopcnt, trans_seq);

		delegator->DeleteOutAgentData(vtAgentQueue);
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("FaxAgent::AgentProc(), 종료됨."));

	delegator->_bDoneThreadFlag = TRUE;

	return 0;
}


UINT FaxAgent::ResultWatchProc(LPVOID lpParameter)
{
	FaxAgent* delegator = (FaxAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.xpediteAgent.dbName;
	CString _dbport = g_config.db.dbport;


	Log(CMdlLog::LEVEL_EVENT, _T("FaxAgent::ResultWatchProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("FaxAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("FaxAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ResultWatchProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	// 20분 간격으로 DB연결 확인
	int idleTime = 0;
	int checkConnectionInterval = (1 * 20 * 60) * 1000;  // millisec

	while (1)
	{
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
			Log(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ResultWatchProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT && idleTime > checkConnectionInterval)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("FaxAgent::ResultWatchProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("FaxAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
						Log(CMdlLog::LEVEL_ERROR, msg);
						return false;
					}
				}
			}	

			idleTime = 0;

			continue;
		}

		if (delegator->processResult(_mysql) == false) 
			idleTime += timeout_inerval;
		else
			idleTime = 0;
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("FaxAgent::ResultWatchProc(), 종료됨."));

	delegator->_bDoneThreadFlag2 = TRUE;

	return 0;
}