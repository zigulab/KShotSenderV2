// LGUPlusAgent.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "LGUPlusAgent.h"
#include "DBManager.h"
#include "sendManager.h"

#include "./include/mysql/errmsg.h"


// LGUPlusAgent

LGUPlusAgent::LGUPlusAgent(DBManager*	dbManager)
{
	_dbManager = dbManager;
}

LGUPlusAgent::~LGUPlusAgent()
{
}


// LGUPlusAgent 멤버 함수

// LGUPlusAgent 멤버 함수
BOOL LGUPlusAgent::Init()
{
	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;
	_bDoneThreadFlag2 = FALSE;

	int useagent = _ttoi(g_config.lguplusAgent.useAgent);
	if (useagent)
	{
		_hRunDelegatorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		_hRunDelegatorEvent2 = CreateEvent(NULL, FALSE, FALSE, NULL);

		AfxBeginThread(AgentProc, this);
		AfxBeginThread(ResultWatchProc, this);
	}

	return TRUE;
}

BOOL LGUPlusAgent::UnInit()
{
	_bExitFlag = TRUE;

	int useagent = _ttoi(g_config.lguplusAgent.useAgent);
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

void LGUPlusAgent::SetNewSendNotify()
{
	while (_agent_state == AS_DOING)
		Sleep(10);

	SetEvent(_hRunDelegatorEvent);
}

void LGUPlusAgent::AddOutAgentData(vector<SEND_QUEUE_INFO*> vtDatas)
{
	_cs2.Lock();

	//_vtOutAgentDatas = vtOutAgentDatas;
	_vtOutAgentDatas.insert(_vtOutAgentDatas.end(), vtDatas.begin(), vtDatas.end());

	_cs2.Unlock();
}

int	LGUPlusAgent::GetOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
{
	_cs2.Lock();

	int count = _vtOutAgentDatas.size();

	vtDatas = _vtOutAgentDatas;

	_cs2.Unlock();

	return count;
}

void	LGUPlusAgent::DeleteOutAgentData(vector<SEND_QUEUE_INFO*>& vtDatas)
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

void LGUPlusAgent::SetKshotNumber(int number)
{
	_cs.Lock();
	_vtKshotNumber.push_back(number);
	_cs.Unlock();
}

CString LGUPlusAgent::MakeKshotNumberList()
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

CString LGUPlusAgent::MakeJobList()
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

void LGUPlusAgent::DeleteKshotNumber(vector<int>list)
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

int LGUPlusAgent::GetResultRow(MYSQL& mysql, CString kshotNumberList)
{
	// 현재일자를 구한다.
	CTime    CurTime = CTime::GetCurrentTime();

	CString  smsTablename = CurTime.Format("sc_log_%Y%m");
	CString  mmsTablename = CurTime.Format("mms_log_%Y%m");

	// sms row count얻기
	int sms_cnt = GetResultRowWithTableName(mysql, kshotNumberList, smsTablename, TRUE);

	// mms row count얻기
	int mms_cnt = GetResultRowWithTableName(mysql, kshotNumberList, mmsTablename, FALSE);

	int row_count = sms_cnt + mms_cnt;

	return row_count;
}

int LGUPlusAgent::GetResultRowWithTableName(MYSQL& mysql, CString kshotNumberList, CString tablename, BOOL isSMS)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	// SMS 처리
	CString query;
	if( isSMS )
		query.Format(_T("select tr_num, tr_etc1 from %s where tr_sendstat = 2 and tr_etc1 in (%s)"), tablename, kshotNumberList);
	else
		query.Format(_T("select msgkey, etc1 from %s where status = 3 and etc1 in (%s)"), tablename, kshotNumberList);

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		// 테이블이 존재하지 않음. ( 일일 처음 발송일 때 )
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResultRowWithTableName(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResultRowWithTableName(), query(%s)"), query);
		return -1;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResultRowWithTableName(), mysql_store_result error(%s)"), mysql_error(&mysql));
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

void LGUPlusAgent::GetResult(MYSQL& mysql, vector<resultPack>& vtResultPack, CString jobList)
{
	// 현재일자를 구한다.
	CTime    CurTime = CTime::GetCurrentTime();

	CString  smsTablename = CurTime.Format("sc_log_%Y%m");
	CString  mmsTablename = CurTime.Format("mms_log_%Y%m");

	// sms 처리
	GetResultWithTableName(mysql, vtResultPack, jobList, smsTablename, TRUE);

	// mms 처리
	GetResultWithTableName(mysql, vtResultPack, jobList, mmsTablename, FALSE);
}

void  LGUPlusAgent::GetResultWithTableName(MYSQL& mysql, vector<resultPack>& vtResultPack, CString jobList, CString tablename, BOOL isSMS)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	
	if(isSMS)
		query.Format(_T("select tr_num, tr_rsltstat, tr_rsltdate, tr_net from %s where tr_num in (%s)"), tablename, jobList);
	else
		query.Format(_T("select msgkey, rslt, terminateddate, telcoinfo from %s where msgkey in (%s)"), tablename, jobList);


	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResult(), mysql_error(%s)"), mysql_error(&mysql));
		query = query.Left(4000);
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResult(), query(%s)"), query);
		return;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::GetResult(), mysql_store_result error(%s)"), mysql_error(&mysql));
		return;
	}

	while ((_row = mysql_fetch_row(_res)) != NULL)
	{
		resultPack rp;

		int msgkey = _ttoi(_row[0]);

		map<int, int>::iterator it = _mpKshotNumber.begin();
		for (; it != _mpKshotNumber.end(); it++)
		{
			if (it->second == msgkey)
			{
				rp.kshot_number = it->first;
				break;
			}
		}

		rp.result = _ttoi(_row[1]);
		strcpy(rp.resultTime, _row[2]);

		//rp.telecom = _ttoi(_row[3]);

		CString Telecom = _row[3];

		// 1: SKT, 2:KT, 3:LG, 4:AHNN, 5:DACOM, 6:SK Broadband, 7 : KTF, 8 : LGT, 0 : Unkwon

		if (isSMS)
		{
			// 통신사 처리
			if (Telecom.CompareNoCase(_T("010")) == 0)
				rp.telecom = 1;
			else if (Telecom.CompareNoCase(_T("011")) == 0)
				rp.telecom = 2;
			else if (Telecom.CompareNoCase(_T("016")) == 0)
				rp.telecom = 2;
			else if (Telecom.CompareNoCase(_T("017")) == 0)
				rp.telecom = 8;
			else if (Telecom.CompareNoCase(_T("109")) == 0)
				rp.telecom = 8;
			else
				rp.telecom = 0;

			// 성공 결과값 변환
			if (rp.result == 6)
				rp.result = 0;
		}
		else
		{
			if (Telecom.CompareNoCase(_T("SKT")) == 0)
				rp.telecom = 1;
			else if (Telecom.CompareNoCase(_T("KT")) == 0)
				rp.telecom = 2;
			else if (Telecom.CompareNoCase(_T("LG")) == 0)
				rp.telecom = 3;
			else if (Telecom.CompareNoCase(_T("AHNN")) == 0)
				rp.telecom = 4;
			else if (Telecom.CompareNoCase(_T("DACOM")) == 0)
				rp.telecom = 5;
			else if (Telecom.CompareNoCase(_T("SK Broadband")) == 0)
				rp.telecom = 6;
			else if (Telecom.CompareNoCase(_T("KTF")) == 0)
				rp.telecom = 7;
			else if (Telecom.CompareNoCase(_T("LGT")) == 0)
				rp.telecom = 8;
			else
				rp.telecom = 0;

			// 성공 결과값 변환
			if (rp.result == 1000)
				rp.result = 0;
		}

		vtResultPack.push_back(rp);
	}

	mysql_free_result(_res);
}

// 결과에 따라 msgResult를 갱신하고 kshotAgent에게 결과를 보낸다.
void LGUPlusAgent::SendAgentResult(vector<resultPack>& vtResultPack)
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

		//int nTelecom;

		//if (rp.telecom == 1)
		//	nTelecom = 1;
		//else if (rp.telecom == 2)
		//	nTelecom = 7;
		//else if (rp.telecom == 3)
		//	nTelecom = 8;
		//else
		//	nTelecom = 0;

		g_sendManager->PushResult(strNumber.GetBuffer(), rp.result, strTime.GetBuffer(), rp.telecom);
	}
}

bool LGUPlusAgent::processResult(MYSQL& mysql, CString kshotNumberList)
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


bool LGUPlusAgent::SetKNumberWithReload(MYSQL& mysql)
{
	MYSQL_RES*		_res;
	MYSQL_ROW		_row;

	CString query;
	query.Format(_T("select kshot_number from tblmessage where fsenddate > DATE_ADD(now(),INTERVAL -2 DAY)"));

	// mysql_query == 0 이면 성공
	if (mysql_query(&mysql, query))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::SetKNumberWithReload(), mysql_query(%s) error(%s)"), query, mysql_error(&mysql));
		return false;
	}

	if ((_res = mysql_store_result(&mysql)) == NULL)
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::SetKNumberWithReload(), mysql_store_result error(%s)"), mysql_error(&mysql));
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


UINT LGUPlusAgent::AgentProc(LPVOID lpParameter)
{
	LGUPlusAgent* delegator = (LGUPlusAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _dbport = g_config.db.dbport;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.lguplusAgent.dbname;

	Log(CMdlLog::LEVEL_EVENT, _T("LGUPlusAgent::AgentProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	int port = _ttoi(_dbport);
	CString msg;
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("LGUPlusAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("LGUPlusAgent::AgentProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), set names euckr error(%s)"), mysql_error(&_mysql));
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
			Log(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), WaitForSingleObject Fail"));
			break;
		}

		if (delegator->_bExitFlag)
			break;

		if (ret == WAIT_TIMEOUT)
		{
			if (mysql_ping(&_mysql) != 0)
			{
				int errorno = mysql_errno(&_mysql);

				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), ping error(%s)"), mysql_error(&_mysql));
				if (errorno == CR_SERVER_GONE_ERROR)
				{
					if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
					{
						CString msg;
						msg.Format(_T("LGUPlusAgent::AgentProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
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

		CString sms_query, lms_query, mms_query;
		sms_query.Format(_T("insert into SC_TRAN(TR_SENDDATE, TR_SENDSTAT, TR_MSGTYPE, TR_PHONE, TR_CALLBACK, TR_MSG, TR_ETC1) values"));

		lms_query.Format(_T("insert into MMS_MSG(SUBJECT, PHONE, CALLBACK, STATUS, REQDATE, MSG, TYPE, ETC1) values"));

		mms_query.Format(_T("insert into MMS_MSG(SUBJECT, PHONE, CALLBACK, STATUS, REQDATE, MSG, FILE_CNT, FILE_PATH1, TYPE, ETC1) values"));

		int cnt = 0, sms_cnt = 0, lms_cnt = 0, mms_cnt = 0;

		vector<SEND_QUEUE_INFO*>::iterator it = vtAgentQueue.begin();
		for (; it != vtAgentQueue.end(); it++)
		{
			SEND_QUEUE_INFO* info = *it;

			CString OneRowValue;

			// 문장내 ' =>\' 변경
			CString smsMessage = info->msg;
			smsMessage.Replace("'", "\\'");

			CString smsSubject = info->subject;
			if (info->msgtype > 0)
				smsSubject.Replace("'", "\\'");

			if (info->msgtype == 0)
			{
				//TR_SENDDATE, TR_SENDSTAT, TR_MSGTYPE, TR_PHONE, TR_CALLBACK, TR_MSG, TR_ETC1

				OneRowValue.Format(_T(" (now(), '0', '0', '%s', '%s', '%s', '%d')"),
													info->receive, info->callback, smsMessage, info->kshot_number
											);

				sms_query += OneRowValue;
				sms_query += ",";

				sms_cnt++;
			}
			else
			{
				if (info->msgtype == 2)
				{
					//SUBJECT, PHONE, CALLBACK, STATUS, REQDATE, MSG, FILE_CNT, FILE_PATH1, TYPE, ETC1

					CString filePath;
					filePath.Format(_T("http://%s:%s%s"), g_config.kshot.mmsServerIP, g_config.kshot.mmsServerPort, info->filepath);
					OneRowValue.Format(_T(" ('%s', '%s', '%s', '0', now(), '%s', 1, '%s', '0', '%d')"),
						smsSubject, info->receive, info->callback,
						smsMessage, filePath, info->kshot_number
					);
					mms_query += OneRowValue;
					mms_query += ",";
					mms_cnt++;
				}
				else
				{
					//SUBJECT, PHONE, CALLBACK, STATUS, REQDATE, MSG, TYPE, ETC1

					OneRowValue.Format(_T(" ('%s', '%s', '%s', '0', now(), '%s', '0', '%d')"),
						smsSubject, info->receive, info->callback,
						smsMessage, info->kshot_number
					);
					lms_query += OneRowValue;
					lms_query += ",";
					lms_cnt++;
				}			
			}

			cnt++;
			delegator->SetKshotNumber(info->kshot_number);
		}

		if (sms_cnt > 0) 
		{
			sms_query.Delete(sms_query.GetLength() - 1);

			if (mysql_query(&_mysql, sms_query))
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), mysql_error(%s)"), mysql_error(&_mysql));
				sms_query = sms_query.Left(4000);
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), smsquery (%s)"), sms_query);
			}
		}

		if (lms_cnt > 0)
		{
			lms_query.Delete(lms_query.GetLength() - 1);

			if (mysql_query(&_mysql, lms_query))
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), mysql_error(%s)"), mysql_error(&_mysql));
				lms_query = lms_query.Left(4000);
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), query (%s)"), lms_query);
			}
		}

		if (mms_cnt > 0)
		{
			mms_query.Delete(mms_query.GetLength() - 1);

			if (mysql_query(&_mysql, mms_query))
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), mysql_error(%s)"), mysql_error(&_mysql));
				mms_query = mms_query.Left(4000);
				Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::AgentProc(), query (%s)"), mms_query);
			}
		}

		CString logmsg;
		logmsg.Format(_T("LGUPlusAgent::AgentProc(), [msg] 외부 Agent모듈로 발송 전달(count:%d)"), cnt);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		delegator->DeleteOutAgentData(vtAgentQueue);
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("LGUPlusAgent::AgentProc(), 종료됨."));

	delegator->_bDoneThreadFlag = TRUE;

	return 0;
}


UINT LGUPlusAgent::ResultWatchProc(LPVOID lpParameter)
{
	LGUPlusAgent* delegator = (LGUPlusAgent*)lpParameter;

	DBManager*	_dbManager = delegator->_dbManager;

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _dbport = g_config.db.dbport;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.lguplusAgent.dbname;
	
	Log(CMdlLog::LEVEL_EVENT, _T("LGUPlusAgent::ResultWatchProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("LGUPlusAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("LGUPlusAgent::ResultWatchProc(), Mysql 연결(%s,%s//%s,%s,%s) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::ResultWatchProc(), set names euckr error(%s)"), mysql_error(&_mysql));
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
			Log(CMdlLog::LEVEL_ERROR, _T("LGUPlusAgent::ResultWatchProc(), WaitForSingleObject Fail"));
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
						msg.Format(_T("LGUPlusAgent::ResultWatchProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
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

	Log(CMdlLog::LEVEL_DEBUG, _T("LGUPlusAgent::ResultWatchProc(), 종료됨."));

	delegator->_bDoneThreadFlag2 = TRUE;

	return 0;
}