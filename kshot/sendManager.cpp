// sendManager.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "kshot.h"
#include "sendManager.h"

#include "./include/mysql/errmsg.h"



bool GetQueueData(MYSQL* mysql, vector<SEND_QUEUE_INFO*>& sendQueue)
{
	MYSQL_RES*		res;
	MYSQL_ROW		row;

	//**********************************************************//
	// 한번에 처리하는 갯수가 중요하다. 속도와 메모리의 관계성이 높다.
	// 제한을 주지 않으면 너무 많은 메모리사용으로 crash가 발생하고
	// 너무 적은수는 잖은 DB Access로 속도가 낮아진다.
	// 적정수준을 찾아야 한다. 현재는 5만개로 제한한다. (Limit 50000)
	//**********************************************************//


	//int limitnum = 50000;

	CString query;

	//query.Format(_T("select * from msgqueue where (state = 0 AND isReserved = 'N') OR (state = 0 AND isReserved = 'Y' AND reservedTime <= NOW())"));
	//query.Format(_T("SELECT *, rownum FROM(SELECT a.*, (CASE @vuid WHEN a.uid THEN @rownum:=@rownum+1 ELSE @rownum:=1 END) rownum, (@vuid:=a.uid) vuid FROM msgqueue a, (SELECT @vuid:='', @rownum:=0 FROM dual) b where (a.state = 0 AND a.isReserved = 'N') OR (a.state = 0 AND a.isReserved = 'Y' AND a.reservedTime <= NOW()) ORDER BY a.uid, a.send_queue_id) c where rownum < 500"));

	/*
	query.Format(_T("SELECT *, rownum \
									FROM \
									( \
										SELECT a.*, \
											(CASE @vuid WHEN a.uid THEN @rownum:=@rownum+1 ELSE @rownum:=1 END) rownum, \
											(@vuid:=a.uid) vuid \
										FROM msgqueue a, (SELECT @vuid:='', @rownum:=0 FROM dual) b \
										where (a.state = 0 AND a.isReserved = 'N') OR (a.state = 0 AND a.isReserved = 'Y' AND a.reservedTime <= NOW()) ORDER BY a.uid, a.send_queue_id \
									) c \
									where rownum < 500"));
	*/
	
	//query.Format(_T("select * from msgqueue where (state = 0 AND isReserved = 'N') OR (state = 0 AND isReserved = 'Y' AND reservedTime <= NOW()) Limit %d"), limitnum);

	// msgqueue에는 예약 건만 등록된다. 여기서는 모두 수집한다.
	query.Format(_T("select * from msgqueue where (state = 0 AND isReserved = 'N') OR (state = 0 AND isReserved = 'Y' AND reservedTime <= NOW())"));

	
	if(mysql_query(mysql, query))
	{
		//Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), mysql_query error(%s)"),mysql_error(mysql));
		return false;
	}

	if((res = mysql_store_result(mysql)) == NULL)
	{ 
		Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), mysql_store_result error(%s)"),mysql_error(mysql));
		return false;
	}

	if ((row = mysql_fetch_row(res)) == NULL) {
		mysql_free_result(res);
		return false;
	}

	 //unsigned long long count = _res->row_count;

	do
	{
		SEND_QUEUE_INFO* qinfo = new SEND_QUEUE_INFO();
		
		//memset(qinfo, 0x00, sizeof(qinfo));
	
		if( row[0] != NULL )
			qinfo->kshot_number = _ttoi(row[0]);

		if( row[1] != NULL )
			qinfo->uid = row[1];

		if( row[3] != NULL )
			qinfo->msgtype = _ttoi(row[3]);

		if( row[2] != NULL )
			qinfo->send_number = row[2];

		if( row[5] != NULL )
			qinfo->callback = row[5];

		if( row[6] != NULL )
			qinfo->receive = row[6];

		if (row[7] != NULL)
			qinfo->subject = row[7];

		if (row[8] != NULL)
			qinfo->msg = row[8];
	
		int mms_id = -1;
		if( row[9] != NULL )
			mms_id = _ttoi(row[9]);

		if( row[10] != NULL )
			qinfo->isReserved = *row[10];
	
		if( row[11] != NULL )
			qinfo->reservedTime = row[11];

		// 라인 찾기
		{
			CString strLine = _T("KPMOBILE");

			MYSQL_RES*		l_res;
			MYSQL_ROW		l_row;

			CString query_l;

			// 대기중인 것만
			query_l.Format(_T("select smsline, lmsline, mmsline, faxline from member where uid = '%s'"), qinfo->uid);
			if (mysql_query(mysql, query_l))
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), select(query_l) error(%s)"), mysql_error(mysql));
				return false;
			}

			if ((l_res = mysql_store_result(mysql)) == NULL)
			{
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), select(query_l)-result2 error(%s)"), mysql_error(mysql));
				return false;
			}

			if ((l_row = mysql_fetch_row(l_res)) == NULL) {
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData() select(query_l)-fetch2 error(%s)"), mysql_error(mysql));
				mysql_free_result(l_res);
				return false;
			}

			if (qinfo->msgtype == 0)
			{
				strLine = l_row[0];
			}
			else if (qinfo->msgtype == 1)
			{
				strLine = l_row[1];
			}
			else if (qinfo->msgtype == 2)
			{
				strLine = l_row[2];
			}
			else if (qinfo->msgtype == 3)
			{
				strLine = l_row[3];
			}

			qinfo->line = strLine;

			mysql_free_result(l_res);
		}

		// 0,1
		if( qinfo->msgtype < 2 )
			sendQueue.push_back(qinfo);
		else //( qinfo->msgtype == 2 ) 3,4
		{
			// mms 파일정보가 없어 다음으로 넘김
			if (mms_id == -1) {
				delete qinfo;
				continue;
			}

			MYSQL_RES*		temp_res;
			MYSQL_ROW		temp_row;

			CString query2;
	
			// 대기중인 것만
			query2.Format(_T("select * from mmsinfo where mmsfile_id = %d"), mms_id);
			if(mysql_query(mysql, query2))
			{ 
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), select2 error(%s)"),mysql_error(mysql));
				return false;
			}

			if((temp_res = mysql_store_result(mysql)) == NULL)
			{ 
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData(), select2-result2 error(%s)"),mysql_error(mysql));
				return false;
			}

			if ((temp_row = mysql_fetch_row(temp_res)) == NULL) {
				Logmsg(CMdlLog::LEVEL_ERROR, _T("GetQueueData() select2-fetch2 error(%s)"), mysql_error(mysql));
				mysql_free_result(temp_res);
				return false;
			}

			if (temp_row[1] != NULL)
				//strcpy(qinfo->filename, temp_row[1]);
				qinfo->filename = temp_row[1];

			if (temp_row[2] != NULL)
				//strcpy(qinfo->filepath, temp_row[2]);
				qinfo->filepath = temp_row[2];

			if( temp_row[3] != NULL )
				qinfo->filesize = _ttoi(temp_row[3]);	

			mysql_free_result(temp_res);

			sendQueue.push_back(qinfo);
		}

	}
	while((row = mysql_fetch_row(res)) != NULL);

	mysql_free_result(res);

	return true;
}

UINT SendMessageProc(LPVOID lpParameter)
{
	SendManager*	_sendManager = (SendManager *)lpParameter;
	CommManager*	_commManager = &_sendManager->_commManager;
	DBManager*		_dbManager = &_sendManager->_dbManager;


	MYSQL		mysql; 

	mysql_init(&mysql);

	CString msg;
	int port = _ttoi(_dbManager->_dbport);
	if(!mysql_real_connect(&mysql, _dbManager->_server, _dbManager->_id, _dbManager->_pwd, _dbManager->_dbname, port, 0, 0))
	{
		msg.Format(_T("SendMessageProc(), mysql 연결 실패(%s)"), mysql_error(&mysql));
		Logmsg(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("SendMessageProc(), mysql 연결(%s,%s//%s,%s,%d) 성공"), _dbManager->_server, _dbManager->_id, _dbManager->_pwd, _dbManager->_dbname, port);
		Logmsg(CMdlLog::LEVEL_EVENT, msg);
	}
	
	if( mysql_query(&mysql,"set names euckr") ) //한글 인식
	{
		Log(CMdlLog::LEVEL_ERROR, _T("SendMessageProc(), mysql set names euckr 실패"));
		mysql_close(&mysql);
		return false;
	}
	
	while(1)
	{
		if( _sendManager->_bExitFlag )
			break;


		// 예약 발송을 위해 5초 간격으로 읽음.
		int timeout_inerval = 5 * 1000;  // millisec

		DWORD ret = WaitForSingleObject(g_hSendEvent, timeout_inerval);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("SendMessageProc(), WaitForSingleObject Fail"));
			break;
		}

		int doCount = 0;
		
		vector<SEND_QUEUE_INFO*> send_queue;

		// 예약 발송과 즉시발송
		BOOL reservedSendTime = FALSE;
		BOOL instantSendTime = TRUE;

		// 실시간 발송
		if (ret == WAIT_OBJECT_0)
		{
			if ((doCount = _sendManager->PopQueue(send_queue)) < 1)
				continue;
		}
		else if (ret == WAIT_TIMEOUT) {		// 예약 발송(timeout 주기에 예약체크/발송)
			
			if (_sendManager->PopQueue(&mysql, send_queue) == FALSE)
				continue;

			doCount = send_queue.size();
			reservedSendTime = TRUE;
			instantSendTime = FALSE;
		}
		else  // WAIT_FAILED
			continue;


		CString logmsg;
		logmsg.Format(_T("[msg] 발송시작(count:%d)"), doCount);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		/*
		// DB 연결을 유지하기 ping 체크, mysql중단시 재연결 시도
		// 2123 * 5 => 대략 10000 => 10000/1000 => 10.xx초
		// 10.xx 초 단위로 체크
		if (_dbManager->checkNReconnect(&mysql) == false) {
			Log(CMdlLog::LEVEL_ERROR, _T("DB연결에 문제가 있습니다."));
			continue;
		}
		*/

		CString szNumber;
		CString idList;

		// 예약 발송 시간이면 
		if (reservedSendTime)
		{
			for (int n = 0; n < doCount; n++)
			{
				SEND_QUEUE_INFO* ginfo = send_queue[n];

				// 예약발송에서는 ID수집한다. 뒤에서 msgQueue에서 삭제를 위해
				if (n == 0)
					szNumber.Format(_T("%d"), ginfo->kshot_number);
				else
					szNumber.Format(_T(",%d"), ginfo->kshot_number);

				idList += szNumber;
			}
		}

		// 예약 발송 시간이면 
		if (reservedSendTime)
			_dbManager->ModifyState_REQUEST(idList);
		else
		{
			// 아닌 즉시발송이면 msgQueue, msgResult에 복사
			_dbManager->InsertMessageTable(send_queue);
		}

		//DWORD b = GetTickCount();

		// 1만건씩 나눠서 처리( 급격한 메모리 증가를 줄이기 위해 )
		vector<vector<SEND_QUEUE_INFO*>> arbuf;
		
		int countInGroup = 10000;
		int group_num = doCount / countInGroup;
		int lastcountInGroup = doCount % countInGroup;
		if(lastcountInGroup != 0 )
			group_num++;

		int bpos = 0, epos = 0;
		if (group_num > 0) {
			for (int i = 0; i < group_num; i++) {
				bpos = i * countInGroup;
				if (i == group_num - 1) 
				{
					if (lastcountInGroup == 0)
						epos = bpos + countInGroup;
					else
						epos = bpos + lastcountInGroup;
				}
				else
					epos = bpos+ countInGroup;

				vector<SEND_QUEUE_INFO*> vt;
				vt.assign(send_queue.begin() + bpos, send_queue.begin() + epos);
				arbuf.push_back(vt);
				TRACE(_T("vtBuf, bpos:%d, epos:%d\n"), bpos, epos);
			}
		}

		// OutAgentQueue;
		vector<SEND_QUEUE_INFO*> outAgentQueue;

		// OutAgentQueue;
		vector<SEND_QUEUE_INFO*> faxAgentQueue;

		// OutAgentQueue;
		vector<SEND_QUEUE_INFO*> haoAgentQueue;

		// OutAgentQueue;
		vector<SEND_QUEUE_INFO*> lguplusAgentQueue;

		for (int gidx = 0; gidx < group_num; gidx++)
		{
			vector<SEND_QUEUE_INFO*> vtgroup = arbuf[gidx];

			int group_count = vtgroup.size();
			for (int n = 0; n < group_count; )
			{
				SEND_QUEUE_INFO* ginfo = vtgroup[n];

				// 즉시 발송시간에, 예약건이면 제외
				if (instantSendTime && toupper(ginfo->isReserved) == 'Y') {
					n++;
					continue;
				}

				// 단문 and 지능망 and "KPMOBILE"라인 일때, 150건씩 스레드 전환 => 지능망의 1초당 150건 제한을 준수한다.
				if (ginfo->msgtype == 0 && g_enable_intelli ) 
				{
					if( stricmp(ginfo->line, "KPMOBILE") == 0) {
						if (n != 0 && n % 150 == 0)
							Sleep(950);
					}
				}


				char number[20] = { 0 };
				sprintf(number, _T("%d"), ginfo->kshot_number);

				// 결과 큐에 하나 넣는다.
				//_sendManager->AddResultQueue(ginfo);

				//Logmsg(CMdlLog::LEVEL_DEBUG, _T("SendMessageProc(), 발송(send_number:%s, kshot_number:%s, callbackNo:%s"), ginfo->send_number, number, ginfo->callback);

				// 발송시작
				///*
				if (ginfo->msgtype == 0) 
				{
					if( stricmp(ginfo->line, "KPMOBILE") == 0 )
						_commManager->SendSMS(number, ginfo->callback, ginfo->receive, ginfo->subject, ginfo->msg);
					else if (stricmp(ginfo->line, "WISECAN") == 0)
					{
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						outAgentQueue.push_back(clone);
					}
					else if (stricmp(ginfo->line, "LGUPLUS") == 0)
					{
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						lguplusAgentQueue.push_back(clone);
					}
					else
						_commManager->SendSMS(number, ginfo->callback, ginfo->receive, ginfo->subject, ginfo->msg);
				}
				else if (ginfo->msgtype == 1)
				{
					if (stricmp(ginfo->line, "LGUPLUS") == 0)
					{
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						lguplusAgentQueue.push_back(clone);
					}
					else
						_commManager->SendLMS(number, ginfo->callback, ginfo->receive, ginfo->subject, ginfo->msg);
				}
				else if (ginfo->msgtype == 2)
				{
					if (stricmp(ginfo->line, "HAOMUN") == 0) {
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						haoAgentQueue.push_back(clone);
					}
					else if (stricmp(ginfo->line, "LGUPLUS") == 0)
					{
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						lguplusAgentQueue.push_back(clone);
					}
					else
						_commManager->SendMMS(number, ginfo->callback, ginfo->receive, ginfo->subject, ginfo->msg, ginfo->filename, ginfo->filepath, ginfo->filesize);
				}
				else if (ginfo->msgtype == 3)
				{
					int isOuterFax = _ttoi(g_config.xpediteAgent.useAgent);
					if (isOuterFax) {
						SEND_QUEUE_INFO *clone = new SEND_QUEUE_INFO(*ginfo);
						faxAgentQueue.push_back(clone);
					}
					else
						_commManager->SendFAX(number, ginfo->callback, ginfo->receive, ginfo->subject, ginfo->msg, ginfo->filename, ginfo->filepath, ginfo->filesize);
				}
				//*/

				n++;

				if (n % 1000 == 0)
				{
					logmsg.Format(_T("[msg] 발송(count:%d)"), (gidx*10000) + n);
					Log(CMdlLog::LEVEL_EVENT, logmsg);
					Sleep(0);
				}
			}

			// 외부모듈(wiseCan)발송이면
			if (!outAgentQueue.empty()) {
				_dbManager->_agentDelegator.AddOutAgentData(outAgentQueue);
				_dbManager->_agentDelegator.SetNewSendNotify();
			}

			// fax모듈(xpedite)발송이면
			if (!faxAgentQueue.empty()) {
				_dbManager->_faxDelegator.AddOutAgentData(faxAgentQueue);
				_dbManager->_faxDelegator.SetNewSendNotify();
			}

			// haomun MMS발송이면
			if (!haoAgentQueue.empty()) {
				_dbManager->_haomunDelegator.AddOutAgentData(haoAgentQueue);
				_dbManager->_haomunDelegator.SetNewSendNotify();
			}

			// lguplus 발송이면
			if (!lguplusAgentQueue.empty()) {
				_dbManager->_lguplusDelegator.AddOutAgentData(lguplusAgentQueue);
				_dbManager->_lguplusDelegator.SetNewSendNotify();
			}

			_sendManager->flushQueue(vtgroup);
		}

		logmsg.Format(_T("[msg] 발송완료(count:%d)"), doCount);
		Log(CMdlLog::LEVEL_EVENT, logmsg);
		
		// 상대적인 느린 처리 시뮬레이션
		//Sleep(500);

		// 초기화한다.
		//_sendManager->flushQueue(send_queue);

		// 미수신 만료 처리는 KshotWatcer로 기능 이관
		// _sendManager->FailForExpireItem();
	}

	mysql_close(&mysql);

	Log(CMdlLog::LEVEL_DEBUG, _T("SendMessageProc(), 종료됨."));

	_sendManager->_bDoneThreadFlag = TRUE;

	return 0;
}

SendManager::SendManager()
{
	_keyinitvalue = 0x10000000;

	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;

	_bDoneResultFlag = FALSE;
}

SendManager::~SendManager()
{
}


// sendManager 멤버 함수

BOOL SendManager::prevInit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("SendManager 초기화"));

	_bExitFlag = FALSE;
	_bDoneThreadFlag = FALSE;

	_bDoneResultFlag = FALSE;

	if (_ttoi(g_config.config.debug_trace) != 0)
		g_bDebugTrace = TRUE;
	else
		g_bDebugTrace = FALSE;

	if (_dbManager.Init())
		return TRUE;
	else
		return FALSE;
}

BOOL SendManager::Init()
{
	if( _commManager.Init() == FALSE )
		return FALSE;

	_clientManager.Init();

	g_hSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	hResultDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager::Init() Running..."));

	CWinThread* pThread = AfxBeginThread(SendMessageProc, this);


	AfxBeginThread(ResultDoneProc, this);

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager 초기화 완료"));

	return TRUE;
}

void SendManager::SetResultDoneEvent()
{
	SetEvent(hResultDoneEvent);
}

BOOL SendManager::Connect()
{
	return _commManager.ConnectToTelecom();
}

BOOL SendManager::Reconnect(TELECOM_SERVER_NAME server)
{
	if(server == TELECOM_KT_XROSHOT)
		_commManager.ConnectToXroShot();
	else if (server == TELECOM_KT_INTELLI)
		_commManager.ConnectToIntelli();

	return TRUE;
}

void SendManager::UnInit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("SendManager 종료화"));

	// 먼저, 클라이언트 연결을 중지한다.
	_clientManager.UnInit();

	// 현재 발송을 중단한다.
	_bExitFlag = TRUE;

	SetEvent(g_hSendEvent);
	SetEvent(hResultDoneEvent);
	while( !_bDoneThreadFlag || !_bDoneResultFlag )
		::Sleep(10);

	// 통신관리를 중지한다.
	_commManager.Uninit();

	// DB관리를 중지한다.
	_dbManager.UnInit();

	
	// 내부적인 데이타 관리를 정리한다.
	vector<RESULT_ITEM*>::iterator it;
	for (it = _msgresult.begin(); it != _msgresult.end(); it++) 
	{
		delete (RESULT_ITEM*)*it;
	}
	_msgresult.clear();

	vector<SEND_QUEUE_INFO*>::iterator it2 = _sendQueue.begin();
	for(; it2 != _sendQueue.end(); it2++ )
	{
		delete *it2;
	}
	_sendQueue.clear();

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager 자료구조 삭제"));

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager 종료화 완료"));
}

void SendManager::SendProcess(CString id, int msgtype, char *send_number, char* callback, char* receive, char* subject, 
								char* msg, char* filename, char* filepath, int filesize, char isReserved, char* reservedTime, char* szRegTime)
{
	//_dbManager.AddSendData(id, send_number, msgtype, callback, receive, subject, msg, 
	//																			filename, filepath, filesize, isReserved, reservedTime, szRegTime);
}

bool SendManager::AddResultQueue(SEND_QUEUE_INFO* info)
{
	//RESULT_ITEM* resultItem = MakeResultItem(info->uid, info->send_number, info->kshot_number);
	//if( !resultItem ) return false;

	//AddResultItem( resultItem );

	_clientManager.SetResultKShotNumber(info->uid, info->send_number, info->kshot_number);

	return true;
}

bool SendManager::PopQueue(MYSQL* mysql, vector<SEND_QUEUE_INFO*>& queue)
{
	bool retval = GetQueueData(mysql, queue);

	return retval;
}

int SendManager::PopQueue(vector<SEND_QUEUE_INFO*>& queue)
{
	_dbManager.GetSendData(queue);

	return queue.size();
}

void SendManager::flushQueue()
{
	vector<SEND_QUEUE_INFO*>::iterator it = _sendQueue.begin();
	for(; it != _sendQueue.end(); it++ )
	{
		delete *it;
	}
	_sendQueue.clear();
}

void SendManager::flushQueue(vector<SEND_QUEUE_INFO*>& queue)
{
	_dbManager.FlushSendData(queue);
}


int	SendManager::GenerateKshotNumber()
{
	static unsigned int count = 0;

	unsigned int kshort_number = 0;

	// 동기화가 필요
	
	kshort_number += (_keyinitvalue + count);
	count++;

	return kshort_number;
}

RESULT_ITEM* SendManager::FindResultKshotNumber(unsigned int kshot_number)
{
	RESULT_ITEM *result = NULL;

	_cs.Lock();
	
	vector<RESULT_ITEM*>::iterator it = _msgresult.begin();
	for(; it != _msgresult.end(); it++)
	{
		RESULT_ITEM *temp = *it;
		if( temp->kshot_number == kshot_number ) {
			result = temp;
			break;
		}
	}

	_cs.Unlock();

	return result;
}

CString SendManager::FindUidFromKshotNumber(unsigned int kshot_number)
{
	CString uid;

	_cs.Lock();

	vector<RESULT_ITEM*>::iterator it = _msgresult.begin();
	for(; it != _msgresult.end(); it++)
	{
		RESULT_ITEM *result = *it;

		if( result->kshot_number == kshot_number ) {
			uid = result->uid;
			break;
		}
	}

	_cs.Unlock();

	return uid;
}

CString SendManager::FindSendNumberFromKshotNumber(unsigned int kshot_number)
{
	CString send_number;

	_cs.Lock();

	vector<RESULT_ITEM*>::iterator it = _msgresult.begin();
	for(; it != _msgresult.end(); it++)
	{
		RESULT_ITEM *result = *it;

		if( result->kshot_number == kshot_number ) {
			send_number = result->send_number;
			break;
		}
	}

	_cs.Unlock();

	return send_number;
}

/*
SEND_ITEM* SendManager::MakeSendItem(CString id, int msgtype, char *send_number, int kshot_number, char* callback, char* receive, char* subject, 
										char* msg, char* filename, char* filepath, int filesize, char isReserved, char* reservedTime)
{
	SEND_ITEM * item = new SEND_ITEM();
	memset(item, 0x00, sizeof(item));

	strncpy(item->uid, id.GetBuffer(0), id.GetLength() );

	item->msgtype = msgtype;

	strncpy(item->send_number, send_number, strlen(send_number) );
	strncpy(item->callback, callback, strlen(callback) );
	strncpy(item->receive, receive, strlen(receive) );
	strncpy(item->subject, subject, strlen(subject) );
	strncpy(item->msg, msg, strlen(msg) );
	
	if( filename != NULL )
		strncpy(item->filename, filename, strlen(filename) );
	if( filepath != NULL )
		strncpy(item->filepath, filepath, strlen(filepath) );

	item->filesize = filesize;
	item->kshot_number = kshot_number;

	if( toupper(isReserved) == 'Y' )
		item->isReserved = true;
	else
		item->isReserved = false;

	strncpy(item->reservedTime, reservedTime, strlen(reservedTime));
	
	return item;
}
*/

bool SendManager::IsExpireReservedTime(SEND_QUEUE_INFO* ginfo)
{
	CString strDate = ginfo->reservedTime;
	COleDateTime reservedTime;
	reservedTime.SetDateTime(atoi(strDate.Left(4)) , atoi(strDate.Mid(5, 2)), atoi(strDate.Mid(8, 2)),
							atoi(strDate.Mid(11, 2)), atoi(strDate.Mid(14, 2)), atoi(strDate.Mid(17, 2)));

	if( reservedTime == NULL )
		return false;

	COleDateTime nowTime(COleDateTime::GetCurrentTime());
	
	COleDateTimeSpan span = reservedTime - nowTime;

	double timeStmp = span.GetTotalSeconds();

	if( timeStmp <= 0L ) 
		return true;

	return false;
}

bool SendManager::CheckInvalidDateFormat(char* dateFormat, COleDateTime *oleTime)
{
	//유효성 검증

	CString strDate = dateFormat;
	
	oleTime->SetDateTime(atoi(strDate.Left(4)) , atoi(strDate.Mid(5, 2)), atoi(strDate.Mid(8, 2)),
							atoi(strDate.Mid(11, 2)), atoi(strDate.Mid(14, 2)), atoi(strDate.Mid(17, 2)));

	// 잘못된 포맷이면 실패처리 
	if( oleTime->GetStatus() != COleDateTime::valid )
		return false;

	COleDateTime nowTime(COleDateTime::GetCurrentTime());
	
	COleDateTimeSpan span = *oleTime - nowTime;

	double timeStmp = span.GetTotalSeconds();

	// 지난 일자이면 실패처리
	if( timeStmp < 0L ) 
	{
		return false;
	}

	return true;
}

RESULT_ITEM* SendManager::MakeResultItem(CString uid, char* send_number, unsigned int kshot_number)
{
	RESULT_ITEM* item = new RESULT_ITEM();
	memset(item, 0x00, sizeof(item));

	strncpy(item->uid, uid, strlen(uid));
	strncpy(item->send_number, send_number, strlen(send_number));	
	item->kshot_number = kshot_number;

	item->result = -1;

	item->regist_time = GetNowTime();

	return item;
}

void SendManager::AddResultItem(RESULT_ITEM * item)
{
	_cs.Lock();
	_msgresult.push_back(item);
	_cs.Unlock();
}


void SendManager::ResponseFromTelecom(char *kshot_number, int result, char* time)
{
	// TODO : DB에 state 변경

	// 접수 완료(2)로 상태 변경
	//_dbManager.ModifyState_RESPONSE(kshot_number);
	_dbManager.AddResponse(kshot_number);

}

void SendManager::PushResult(char* kshot_number, int result, char* time, int telecom)
{
	////////////////////////////////////////////////////////////////////
	// 결과수신에 대한 DB처리
	////////////////////////////////////////////////////////////////////
	_dbManager.AddResult(kshot_number, result, telecom);

	////////////////////////////////////////////////////////////////////
	// 결과 수신에 대한 Agent에게 전송 또는 실패에 대한 환불처리
	////////////////////////////////////////////////////////////////////
	CString uid;
	CString sendNo;
	CString receiveNo;
	int msgtype = -1;

	float price = 0.0;
	double balance = 0.0;
	CString pid;
	CString line;

	// 등록된 uid-sendno 찾기
	CString uid_sendno = _clientManager.GetUidSendNumberByKShotKey(_ttoi(kshot_number));

	// 등록된 uid-sendno 있으면
	if (uid_sendno.IsEmpty() == FALSE )
	{
		// 결과 설정 => Agent로 리턴
		_clientManager.SetResult(uid_sendno, _ttoi(kshot_number), result, time, telecom);
	}
	else
	{
		// 등록된 uid-sendno 없으면
		Logmsg(CMdlLog::LEVEL_DEBUG, _T("SendManager::PushResult(),  None uid-sendno(kshotNumber:%s"), kshot_number);

		// DB에서 uid, sendNo, receiveNo, msgtype 구함.
		if (_dbManager.GetSendNo(kshot_number, uid, sendNo, receiveNo, msgtype) == false) 
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::PushResult(), _dbManager.GetSendNo() faild"));
		}
		else 
		{
			// 연결된 Client가 없으면 바로 종료
			Client* pClient = _clientManager.GetClient(uid);
			if (pClient == NULL)
			{
				//Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::PushResult(), pClient == NULL"));
				Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::PushResult(), pClient == NULL"));
			}
			else
			{
				uid_sendno.Format(_T("%s-%s"), uid, sendNo);

				//_dbManager.CheckBalance(_id, msgtype, count, &price, pid, line));

				// 실패에 대해서만 price, pid, line 구함.( 환불을 위해 )
				if (result != 0)
					_dbManager.CheckBalance(uid, msgtype, 1, &price, &balance, pid, line);

				// 결과등록
				_clientManager.AddResult(pClient, _ttoi(kshot_number), sendNo, receiveNo, msgtype, price, pid);

				// 결과갱신
				_clientManager.SetResult(uid_sendno, _ttoi(kshot_number), result, time, telecom);
			}
		}
	}

	// 실패에 대해 환불처리
	if (result != 0) 
	{
		// uid 와 단가 정보 얻기
		if( uid.IsEmpty() )
			_clientManager.GetId_Price_MsgType(uid_sendno, _ttoi(kshot_number), uid, pid, &price, &msgtype);

		// 환불처리
		// 건별로 Update문을 실행하기 때문에 실패가 많은 테스트발송에서는 UI가 Hold되는 현상(먹통)이 발생한다.
		_dbManager.refundBalance(uid, pid, msgtype, price);
	}
	
}

bool SendManager::GetResult(CString id, char* send_number, int *result, char* time, int* telecom)
{
	bool retval = false;

	_cs.Lock();

	vector<RESULT_ITEM*>::iterator it = _msgresult.begin();
	for(; it != _msgresult.end(); it++ ) 
	{
		RESULT_ITEM *item = *it;

		if( item->uid == id && 
			item->result != -1 &&
			strcmp(send_number, item->send_number) == 0 )
		{
			*result = item->result;
			strncpy(time, item->done_time, strlen(item->done_time));
			*telecom = item->telecom;

			delete item;
			_msgresult.erase(it);

			retval = true;

			break;
		}
	}

	_cs.Unlock();

	return retval;
}

time_t SendManager::GetNowTime()
{
	time_t now;
	time_t *ptr_now = &now;
	time(ptr_now);

	return *ptr_now;
}

void SendManager::FailForExpireItem()
{
	time_t now = GetNowTime();

	_cs.Lock();

	vector<RESULT_ITEM*>::iterator it = _msgresult.begin();
	for(; it != _msgresult.end(); it++)
	{
		RESULT_ITEM* item = *it;

		// 대기중인 큐 item이 
		// 하루 만기를 지나면 : 86400(24*60*60)초
		if( (item->regist_time - now) > 86400 )
		{
			// db 실패 처리
			CString knumber;
			knumber.Format(_T("%d"), item->kshot_number);
			_dbManager.FailForExpireSMS(knumber);

			// 항목 삭제
			delete item;
			it = _msgresult.erase(it);
		}
	}

	_cs.Unlock();
}

//CString SendManager::NewVirtualId(CString uid, CString send_number)
//{
//	// 새로운 조합된 kshot_number 생성
//	CString virtualId;
//
//	time_t now;
//	time_t *ptr_now = &now;
//	time(ptr_now);
//
//	CString strTime;
//	strTime.Format(_T("%d"), *ptr_now);
//
//	virtualId.Format(_T("%s_%s_%s"), uid, strTime, send_number);
//
//	return virtualId;
//}

//CString SendManager::GetSendNumberFromKshotNumber(char *kshot_number)
//{
//	// 세번째가 send_number, 두번째는 임의시간
//	CString uid;
//	AfxExtractSubString( uid, kshot_number, 2, '_');
//
//	return uid;
//}


UINT SendManager::ResultDoneProc(LPVOID lpParameter)
{
	SendManager* sendManager = (SendManager*)lpParameter;

	DBManager*	_dbManager = &(sendManager->_dbManager);

	MYSQL			_mysql;
	MYSQL_RES*	_res;
	MYSQL_ROW	_row;

	CString _server = g_config.db.server;
	CString _dbport = g_config.db.dbport;
	CString _id = g_config.db.id;
	CString _pwd = g_config.db.password;
	CString _dbname = g_config.db.dbname;


	Log(CMdlLog::LEVEL_EVENT, _T("SendManager::ResultDoneProc(), Mysql 연결(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("SendManager::ResultDoneProc(), Mysql 연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("SendManager::ResultDoneProc(), Mysql 연결(%s,%s//%s,%s,%d) 성공"), _server, _id, _pwd, _dbname, _dbport);
		Log(CMdlLog::LEVEL_EVENT, msg);
	}

	if (mysql_query(&_mysql, "set names euckr"))
	{
		Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::ResultDoneProc(), set names euckr error(%s)"), mysql_error(&_mysql));
		return -1;
	}

	while (1)
	{
		if (sendManager->_bExitFlag)
			break;

		DWORD ret = WaitForSingleObject(sendManager->hResultDoneEvent, INFINITE);
		switch (ret)
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
			break;
		case WAIT_FAILED:
			Log(CMdlLog::LEVEL_ERROR, _T("SendManager::ResultDoneProc(), WaitForSingleObject Fail"));
			break;
		}

		if (sendManager->_bExitFlag)
			break;

		if (mysql_ping(&_mysql) != 0)
		{
			int errorno = mysql_errno(&_mysql);

			Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::ResultDoneProc(), ping error(%s)"), mysql_error(&_mysql));
			if (errorno == CR_SERVER_GONE_ERROR)
			{
				if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
				{
					CString msg;
					msg.Format(_T("SendManager::ResultDoneProc(), Mysql 재연결(%s,%s//%s,%s,%s) 실패(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
					Log(CMdlLog::LEVEL_ERROR, msg);
					return false;
				}
			}
		}


		// msgresult_todone 테이블의 row를 구해서 결과 처리한다.
		CString query;
		query.Format(_T("select send_queue_id, result, resultTime, now(), telecom from msgresult_todone"));

		// mysql_query == 0 이면 성공
		if (mysql_query(&_mysql, query) == 0)
		{
			if ((_res = mysql_store_result(&_mysql)) != NULL)
			{
				int count = 0;
				while ((_row = mysql_fetch_row(_res)) != NULL)
				{
					CString kshotNumber;
					CString resultTime;
					int result = 9999;
					int telecom = 0;

					kshotNumber	= _row[0];
					result				= _ttoi(_row[1]);

					resultTime		= _row[2];
					if (resultTime.IsEmpty())
						resultTime = _row[3];
					
					telecom = _ttoi(_row[4]);

					sendManager->PushResult(kshotNumber.GetBuffer(), result, resultTime.GetBuffer(), telecom);
					count++;

					if (count % 100 == 0)
						Sleep(5);
				}

				mysql_free_result(_res);
			}
			else
				Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::GetSendNo(), mysql_store_result null"));
		}
		else
			Logmsg(CMdlLog::LEVEL_ERROR, _T("DBManager::GetSendNo(), mysql_query(%s) error(%s)"), query, mysql_error(&_mysql));

		
	}

	Log(CMdlLog::LEVEL_DEBUG, _T("SendManager::ResultDoneProc(), 종료됨."));

	sendManager->_bDoneResultFlag = TRUE;

	return 0;
}