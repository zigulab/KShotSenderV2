// sendManager.cpp : ���� �����Դϴ�.
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
	// �ѹ��� ó���ϴ� ������ �߿��ϴ�. �ӵ��� �޸��� ���輺�� ����.
	// ������ ���� ������ �ʹ� ���� �޸𸮻������ crash�� �߻��ϰ�
	// �ʹ� �������� ���� DB Access�� �ӵ��� ��������.
	// ���������� ã�ƾ� �Ѵ�. ����� 5������ �����Ѵ�. (Limit 50000)
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

	// msgqueue���� ���� �Ǹ� ��ϵȴ�. ���⼭�� ��� �����Ѵ�.
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

		// ���� ã��
		{
			CString strLine = _T("KPMOBILE");

			MYSQL_RES*		l_res;
			MYSQL_ROW		l_row;

			CString query_l;

			// ������� �͸�
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
			// mms ���������� ���� �������� �ѱ�
			if (mms_id == -1) {
				delete qinfo;
				continue;
			}

			MYSQL_RES*		temp_res;
			MYSQL_ROW		temp_row;

			CString query2;
	
			// ������� �͸�
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
		msg.Format(_T("SendMessageProc(), mysql ���� ����(%s)"), mysql_error(&mysql));
		Logmsg(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("SendMessageProc(), mysql ����(%s,%s//%s,%s,%d) ����"), _dbManager->_server, _dbManager->_id, _dbManager->_pwd, _dbManager->_dbname, port);
		Logmsg(CMdlLog::LEVEL_EVENT, msg);
	}
	
	if( mysql_query(&mysql,"set names euckr") ) //�ѱ� �ν�
	{
		Log(CMdlLog::LEVEL_ERROR, _T("SendMessageProc(), mysql set names euckr ����"));
		mysql_close(&mysql);
		return false;
	}
	
	while(1)
	{
		if( _sendManager->_bExitFlag )
			break;


		// ���� �߼��� ���� 5�� �������� ����.
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

		// ���� �߼۰� ��ù߼�
		BOOL reservedSendTime = FALSE;
		BOOL instantSendTime = TRUE;

		// �ǽð� �߼�
		if (ret == WAIT_OBJECT_0)
		{
			if ((doCount = _sendManager->PopQueue(send_queue)) < 1)
				continue;
		}
		else if (ret == WAIT_TIMEOUT) {		// ���� �߼�(timeout �ֱ⿡ ����üũ/�߼�)
			
			if (_sendManager->PopQueue(&mysql, send_queue) == FALSE)
				continue;

			doCount = send_queue.size();
			reservedSendTime = TRUE;
			instantSendTime = FALSE;
		}
		else  // WAIT_FAILED
			continue;


		CString logmsg;
		logmsg.Format(_T("[msg] �߼۽���(count:%d)"), doCount);
		Log(CMdlLog::LEVEL_EVENT, logmsg);

		/*
		// DB ������ �����ϱ� ping üũ, mysql�ߴܽ� �翬�� �õ�
		// 2123 * 5 => �뷫 10000 => 10000/1000 => 10.xx��
		// 10.xx �� ������ üũ
		if (_dbManager->checkNReconnect(&mysql) == false) {
			Log(CMdlLog::LEVEL_ERROR, _T("DB���ῡ ������ �ֽ��ϴ�."));
			continue;
		}
		*/

		CString szNumber;
		CString idList;

		// ���� �߼� �ð��̸� 
		if (reservedSendTime)
		{
			for (int n = 0; n < doCount; n++)
			{
				SEND_QUEUE_INFO* ginfo = send_queue[n];

				// ����߼ۿ����� ID�����Ѵ�. �ڿ��� msgQueue���� ������ ����
				if (n == 0)
					szNumber.Format(_T("%d"), ginfo->kshot_number);
				else
					szNumber.Format(_T(",%d"), ginfo->kshot_number);

				idList += szNumber;
			}
		}

		// ���� �߼� �ð��̸� 
		if (reservedSendTime)
			_dbManager->ModifyState_REQUEST(idList);
		else
		{
			// �ƴ� ��ù߼��̸� msgQueue, msgResult�� ����
			_dbManager->InsertMessageTable(send_queue);
		}

		//DWORD b = GetTickCount();

		// 1���Ǿ� ������ ó��( �ް��� �޸� ������ ���̱� ���� )
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

				// ��� �߼۽ð���, ������̸� ����
				if (instantSendTime && toupper(ginfo->isReserved) == 'Y') {
					n++;
					continue;
				}

				// �ܹ� and ���ɸ� and "KPMOBILE"���� �϶�, 150�Ǿ� ������ ��ȯ => ���ɸ��� 1�ʴ� 150�� ������ �ؼ��Ѵ�.
				if (ginfo->msgtype == 0 && g_enable_intelli ) 
				{
					if( stricmp(ginfo->line, "KPMOBILE") == 0) {
						if (n != 0 && n % 150 == 0)
							Sleep(950);
					}
				}


				char number[20] = { 0 };
				sprintf(number, _T("%d"), ginfo->kshot_number);

				// ��� ť�� �ϳ� �ִ´�.
				//_sendManager->AddResultQueue(ginfo);

				//Logmsg(CMdlLog::LEVEL_DEBUG, _T("SendMessageProc(), �߼�(send_number:%s, kshot_number:%s, callbackNo:%s"), ginfo->send_number, number, ginfo->callback);

				// �߼۽���
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
					logmsg.Format(_T("[msg] �߼�(count:%d)"), (gidx*10000) + n);
					Log(CMdlLog::LEVEL_EVENT, logmsg);
					Sleep(0);
				}
			}

			// �ܺθ��(wiseCan)�߼��̸�
			if (!outAgentQueue.empty()) {
				_dbManager->_agentDelegator.AddOutAgentData(outAgentQueue);
				_dbManager->_agentDelegator.SetNewSendNotify();
			}

			// fax���(xpedite)�߼��̸�
			if (!faxAgentQueue.empty()) {
				_dbManager->_faxDelegator.AddOutAgentData(faxAgentQueue);
				_dbManager->_faxDelegator.SetNewSendNotify();
			}

			// haomun MMS�߼��̸�
			if (!haoAgentQueue.empty()) {
				_dbManager->_haomunDelegator.AddOutAgentData(haoAgentQueue);
				_dbManager->_haomunDelegator.SetNewSendNotify();
			}

			// lguplus �߼��̸�
			if (!lguplusAgentQueue.empty()) {
				_dbManager->_lguplusDelegator.AddOutAgentData(lguplusAgentQueue);
				_dbManager->_lguplusDelegator.SetNewSendNotify();
			}

			_sendManager->flushQueue(vtgroup);
		}

		logmsg.Format(_T("[msg] �߼ۿϷ�(count:%d)"), doCount);
		Log(CMdlLog::LEVEL_EVENT, logmsg);
		
		// ������� ���� ó�� �ùķ��̼�
		//Sleep(500);

		// �ʱ�ȭ�Ѵ�.
		//_sendManager->flushQueue(send_queue);

		// �̼��� ���� ó���� KshotWatcer�� ��� �̰�
		// _sendManager->FailForExpireItem();
	}

	mysql_close(&mysql);

	Log(CMdlLog::LEVEL_DEBUG, _T("SendMessageProc(), �����."));

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


// sendManager ��� �Լ�

BOOL SendManager::prevInit()
{
	Log(CMdlLog::LEVEL_EVENT, _T("SendManager �ʱ�ȭ"));

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

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager �ʱ�ȭ �Ϸ�"));

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
	Log(CMdlLog::LEVEL_EVENT, _T("SendManager ����ȭ"));

	// ����, Ŭ���̾�Ʈ ������ �����Ѵ�.
	_clientManager.UnInit();

	// ���� �߼��� �ߴ��Ѵ�.
	_bExitFlag = TRUE;

	SetEvent(g_hSendEvent);
	SetEvent(hResultDoneEvent);
	while( !_bDoneThreadFlag || !_bDoneResultFlag )
		::Sleep(10);

	// ��Ű����� �����Ѵ�.
	_commManager.Uninit();

	// DB������ �����Ѵ�.
	_dbManager.UnInit();

	
	// �������� ����Ÿ ������ �����Ѵ�.
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

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager �ڷᱸ�� ����"));

	Log(CMdlLog::LEVEL_EVENT, _T("SendManager ����ȭ �Ϸ�"));
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

	// ����ȭ�� �ʿ�
	
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
	//��ȿ�� ����

	CString strDate = dateFormat;
	
	oleTime->SetDateTime(atoi(strDate.Left(4)) , atoi(strDate.Mid(5, 2)), atoi(strDate.Mid(8, 2)),
							atoi(strDate.Mid(11, 2)), atoi(strDate.Mid(14, 2)), atoi(strDate.Mid(17, 2)));

	// �߸��� �����̸� ����ó�� 
	if( oleTime->GetStatus() != COleDateTime::valid )
		return false;

	COleDateTime nowTime(COleDateTime::GetCurrentTime());
	
	COleDateTimeSpan span = *oleTime - nowTime;

	double timeStmp = span.GetTotalSeconds();

	// ���� �����̸� ����ó��
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
	// TODO : DB�� state ����

	// ���� �Ϸ�(2)�� ���� ����
	//_dbManager.ModifyState_RESPONSE(kshot_number);
	_dbManager.AddResponse(kshot_number);

}

void SendManager::PushResult(char* kshot_number, int result, char* time, int telecom)
{
	////////////////////////////////////////////////////////////////////
	// ������ſ� ���� DBó��
	////////////////////////////////////////////////////////////////////
	_dbManager.AddResult(kshot_number, result, telecom);

	////////////////////////////////////////////////////////////////////
	// ��� ���ſ� ���� Agent���� ���� �Ǵ� ���п� ���� ȯ��ó��
	////////////////////////////////////////////////////////////////////
	CString uid;
	CString sendNo;
	CString receiveNo;
	int msgtype = -1;

	float price = 0.0;
	double balance = 0.0;
	CString pid;
	CString line;

	// ��ϵ� uid-sendno ã��
	CString uid_sendno = _clientManager.GetUidSendNumberByKShotKey(_ttoi(kshot_number));

	// ��ϵ� uid-sendno ������
	if (uid_sendno.IsEmpty() == FALSE )
	{
		// ��� ���� => Agent�� ����
		_clientManager.SetResult(uid_sendno, _ttoi(kshot_number), result, time, telecom);
	}
	else
	{
		// ��ϵ� uid-sendno ������
		Logmsg(CMdlLog::LEVEL_DEBUG, _T("SendManager::PushResult(),  None uid-sendno(kshotNumber:%s"), kshot_number);

		// DB���� uid, sendNo, receiveNo, msgtype ����.
		if (_dbManager.GetSendNo(kshot_number, uid, sendNo, receiveNo, msgtype) == false) 
		{
			Logmsg(CMdlLog::LEVEL_ERROR, _T("SendManager::PushResult(), _dbManager.GetSendNo() faild"));
		}
		else 
		{
			// ����� Client�� ������ �ٷ� ����
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

				// ���п� ���ؼ��� price, pid, line ����.( ȯ���� ���� )
				if (result != 0)
					_dbManager.CheckBalance(uid, msgtype, 1, &price, &balance, pid, line);

				// ������
				_clientManager.AddResult(pClient, _ttoi(kshot_number), sendNo, receiveNo, msgtype, price, pid);

				// �������
				_clientManager.SetResult(uid_sendno, _ttoi(kshot_number), result, time, telecom);
			}
		}
	}

	// ���п� ���� ȯ��ó��
	if (result != 0) 
	{
		// uid �� �ܰ� ���� ���
		if( uid.IsEmpty() )
			_clientManager.GetId_Price_MsgType(uid_sendno, _ttoi(kshot_number), uid, pid, &price, &msgtype);

		// ȯ��ó��
		// �Ǻ��� Update���� �����ϱ� ������ ���а� ���� �׽�Ʈ�߼ۿ����� UI�� Hold�Ǵ� ����(����)�� �߻��Ѵ�.
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

		// ������� ť item�� 
		// �Ϸ� ���⸦ ������ : 86400(24*60*60)��
		if( (item->regist_time - now) > 86400 )
		{
			// db ���� ó��
			CString knumber;
			knumber.Format(_T("%d"), item->kshot_number);
			_dbManager.FailForExpireSMS(knumber);

			// �׸� ����
			delete item;
			it = _msgresult.erase(it);
		}
	}

	_cs.Unlock();
}

//CString SendManager::NewVirtualId(CString uid, CString send_number)
//{
//	// ���ο� ���յ� kshot_number ����
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
//	// ����°�� send_number, �ι�°�� ���ǽð�
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


	Log(CMdlLog::LEVEL_EVENT, _T("SendManager::ResultDoneProc(), Mysql ����(mysql_init(), mysql_real_connect())"));

	mysql_init(&_mysql);

	CString msg;
	int port = _ttoi(_dbport);
	if (!mysql_real_connect(&_mysql, _server, _id, _pwd, _dbname, port, 0, 0))
	{
		msg.Format(_T("SendManager::ResultDoneProc(), Mysql ����(%s,%s//%s,%s,%s) ����(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
		Log(CMdlLog::LEVEL_ERROR, msg);
		return false;
	}
	else
	{
		msg.Format(_T("SendManager::ResultDoneProc(), Mysql ����(%s,%s//%s,%s,%d) ����"), _server, _id, _pwd, _dbname, _dbport);
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
					msg.Format(_T("SendManager::ResultDoneProc(), Mysql �翬��(%s,%s//%s,%s,%s) ����(%s)"), _server, _id, _pwd, _dbname, _dbport, mysql_error(&_mysql));
					Log(CMdlLog::LEVEL_ERROR, msg);
					return false;
				}
			}
		}


		// msgresult_todone ���̺��� row�� ���ؼ� ��� ó���Ѵ�.
		CString query;
		query.Format(_T("select send_queue_id, result, resultTime, now(), telecom from msgresult_todone"));

		// mysql_query == 0 �̸� ����
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

	Log(CMdlLog::LEVEL_DEBUG, _T("SendManager::ResultDoneProc(), �����."));

	sendManager->_bDoneResultFlag = TRUE;

	return 0;
}